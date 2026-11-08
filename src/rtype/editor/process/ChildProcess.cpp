/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ChildProcess
*/

#include "ChildProcess.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#endif

#ifdef __APPLE__
#include <crt_externs.h>
#elifndef _WIN32
extern char**
    environ;  // NOLINT(readability-redundant-declaration,misc-use-internal-linkage,cppcoreguidelines-avoid-non-const-global-variables)
#endif

namespace rtype::editor::process {

#ifndef _WIN32
namespace {
/// @return The environment the child inherits: the editor's.
char** processEnvironment() {
#ifdef __APPLE__
    return *_NSGetEnviron();  // `environ` is not visible to a library on macOS
#else
    return environ;
#endif
}
}  // namespace
#endif

namespace {
constexpr auto kPollDelay = std::chrono::milliseconds{5};
}  // namespace

#ifdef _WIN32

namespace {
/// @return @p text quoted as one argument of a Windows command line.
std::string quoteArgument(const std::string& text) {
    if (!text.empty() && text.find_first_of(" \t\n\v\"") == std::string::npos) {
        return text;
    }
    std::string quoted = "\"";
    std::size_t backslashes = 0;
    for (const char c : text) {
        if (c == '\\') {
            ++backslashes;
            continue;
        }
        if (c == '"') {
            quoted.append(backslashes * 2 + 1, '\\');
        } else {
            quoted.append(backslashes, '\\');
        }
        backslashes = 0;
        quoted += c;
    }
    quoted.append(backslashes * 2, '\\');
    return quoted + "\"";
}
}  // namespace

class ChildProcess::Impl {
  public:
    Impl(HANDLE process, HANDLE output, DWORD id) : _process(process), _output(output), _id(id) {}
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl() {
        if (_process != nullptr) {
            CloseHandle(_process);
        }
        if (_output != nullptr) {
            CloseHandle(_output);
        }
    }

    [[nodiscard]] std::uint64_t pid() const { return _id; }

    bool isRunning() {
        if (_exit.has_value()) {
            return false;
        }
        DWORD code = 0;
        if (GetExitCodeProcess(_process, &code) != 0 && code != STILL_ACTIVE) {
            _exit = static_cast<int>(code);
            return false;
        }
        return true;
    }

    std::optional<int> exitCode() {
        (void)isRunning();
        return _exit;
    }

    void requestStop() { kill(); }

    void kill() {
        if (isRunning()) {
            TerminateProcess(_process, 1);
            WaitForSingleObject(_process, 2000);
            (void)isRunning();
        }
    }

    [[nodiscard]] std::string drainOutput() {
        std::string out;
        std::array<char, 4096> buffer{};
        for (;;) {
            DWORD available = 0;
            if (PeekNamedPipe(_output, nullptr, 0, nullptr, &available, nullptr) == 0 || available == 0) {
                break;
            }
            DWORD read = 0;
            if (ReadFile(_output, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) == 0 || read == 0) {
                break;
            }
            out.append(buffer.data(), read);
        }
        return out;
    }

    static std::expected<std::unique_ptr<Impl>, std::string> start(const ProcessSpec& spec) {
        SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        HANDLE readEnd = nullptr;
        HANDLE writeEnd = nullptr;
        if (CreatePipe(&readEnd, &writeEnd, &attributes, 0) == 0) {
            return std::unexpected("cannot create the output pipe");
        }
        SetHandleInformation(readEnd, HANDLE_FLAG_INHERIT, 0);

        std::string commandLine = quoteArgument(spec.program.string());
        for (const std::string& arg : spec.args) {
            commandLine += ' ' + quoteArgument(arg);
        }
        STARTUPINFOA startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = nullptr;
        startup.hStdOutput = writeEnd;
        startup.hStdError = writeEnd;
        PROCESS_INFORMATION info{};
        const std::string directory = spec.workingDirectory.string();
        const BOOL created = CreateProcessA(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                            nullptr, directory.empty() ? nullptr : directory.c_str(), &startup, &info);
        CloseHandle(writeEnd);
        if (created == 0) {
            CloseHandle(readEnd);
            return std::unexpected("cannot start " + spec.program.string());
        }
        CloseHandle(info.hThread);
        return std::make_unique<Impl>(info.hProcess, readEnd, info.dwProcessId);
    }

  private:
    HANDLE _process;
    HANDLE _output;
    DWORD _id;
    std::optional<int> _exit;
};

#else

// The POSIX names come from <sys/types.h>, <sys/wait.h>, <csignal> through headers the include checker does not map.
// NOLINTBEGIN(misc-include-cleaner)
class ChildProcess::Impl {
  public:
    Impl(pid_t id, int output) : _id(id), _output(output) {}
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl() {
        if (_output >= 0) {
            ::close(_output);
        }
    }

    [[nodiscard]] std::uint64_t pid() const { return static_cast<std::uint64_t>(_id); }

    // The wait macros come from <sys/wait.h> through headers the include checker does not map back to it.
    // NOLINTBEGIN(misc-include-cleaner)
    bool isRunning() {
        if (_exit.has_value()) {
            return false;
        }
        int status = 0;
        const pid_t result = ::waitpid(_id, &status, WNOHANG);
        if (result == 0) {
            return true;
        }
        if (result < 0 && errno == EINTR) {
            return true;
        }
        if (result < 0) {
            _exit = -1;  // someone else reaped it
            return false;
        }
        if (WIFEXITED(status)) {
            _exit = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            _exit = -WTERMSIG(status);
        } else {
            return true;  // stopped, not ended
        }
        return false;
    }
    // NOLINTEND(misc-include-cleaner)

    std::optional<int> exitCode() {
        (void)isRunning();
        return _exit;
    }

    void requestStop() {
        if (isRunning()) {
            ::kill(_id, SIGTERM);
        }
    }

    void kill() {
        if (!isRunning()) {
            return;
        }
        ::kill(_id, SIGKILL);
        int status = 0;
        while (::waitpid(_id, &status, 0) < 0 && errno == EINTR) {
        }
        _exit = WIFSIGNALED(status) ? -WTERMSIG(status) : WEXITSTATUS(status);
    }

    [[nodiscard]] std::string drainOutput() const {
        std::string out;
        std::array<char, 4096> buffer{};
        for (;;) {
            const ssize_t count = ::read(_output, buffer.data(), buffer.size());
            if (count > 0) {
                out.append(buffer.data(), static_cast<std::size_t>(count));
            } else if (count < 0 && errno == EINTR) {
                continue;
            } else {
                break;  // nothing more for now (EAGAIN), or the pipe is closed
            }
        }
        return out;
    }

    static std::expected<std::unique_ptr<Impl>, std::string> start(const ProcessSpec& spec) {
        std::array<int, 2> fds{};
        if (::pipe(fds.data()) != 0) {
            return std::unexpected(std::string{"cannot create the output pipe: "} + std::strerror(errno));
        }
        // fcntl is a C variadic function and its flags are macros: nothing else sets a pipe non-blocking.
        // NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,hicpp-vararg,bugprone-signed-bitwise)
        ::fcntl(fds.at(0), F_SETFD, FD_CLOEXEC);
        ::fcntl(fds.at(0), F_SETFL, ::fcntl(fds.at(0), F_GETFL) | O_NONBLOCK);
        // NOLINTEND(cppcoreguidelines-pro-type-vararg,hicpp-vararg,bugprone-signed-bitwise)

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
        posix_spawn_file_actions_adddup2(&actions, fds.at(1), STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, fds.at(1), STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, fds.at(1));

        const std::string program = spec.program.string();
        std::vector<std::string> storage;
        storage.push_back(program);
        storage.insert(storage.end(), spec.args.begin(), spec.args.end());
        std::vector<char*> argv;
        argv.reserve(storage.size() + 1);
        for (std::string& arg : storage) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        // posix_spawn has no portable chdir: the _np action exists on glibc 2.29+ and macOS 10.15+.
        if (!spec.workingDirectory.empty()) {
#if defined(__APPLE__) || defined(__GLIBC__)
            posix_spawn_file_actions_addchdir_np(&actions, spec.workingDirectory.c_str());
#else
            posix_spawn_file_actions_destroy(&actions);
            ::close(fds.at(0));
            ::close(fds.at(1));
            return std::unexpected("a working folder is not supported on this system");
#endif
        }
        pid_t id = 0;
        const bool search = !program.contains('/');
        const int error =
            search ? ::posix_spawnp(&id, program.c_str(), &actions, nullptr, argv.data(), processEnvironment())
                   : ::posix_spawn(&id, program.c_str(), &actions, nullptr, argv.data(), processEnvironment());
        posix_spawn_file_actions_destroy(&actions);
        ::close(fds.at(1));
        if (error != 0) {
            ::close(fds.at(0));
            return std::unexpected("cannot start " + program + ": " + std::strerror(error));
        }
        return std::make_unique<Impl>(id, fds.at(0));
    }

  private:
    pid_t _id;
    int _output;
    std::optional<int> _exit;
};

// NOLINTEND(misc-include-cleaner)

#endif

std::expected<ChildProcess, std::string> ChildProcess::spawn(const ProcessSpec& spec) {
    auto impl = Impl::start(spec);
    if (!impl) {
        return std::unexpected(std::move(impl.error()));
    }
    return ChildProcess{std::move(*impl)};
}

ChildProcess::ChildProcess(std::unique_ptr<Impl> impl) : _impl(std::move(impl)) {}

ChildProcess::ChildProcess(ChildProcess&& other) noexcept = default;

ChildProcess& ChildProcess::operator=(ChildProcess&& other) noexcept {
    if (this != &other) {
        if (_impl) {
            stop();
        }
        _impl = std::move(other._impl);
    }
    return *this;
}

ChildProcess::~ChildProcess() {
    if (_impl) {
        stop();
    }
}

std::uint64_t ChildProcess::pid() const { return _impl ? _impl->pid() : 0; }

bool ChildProcess::isRunning() { return _impl && _impl->isRunning(); }

std::optional<int> ChildProcess::exitCode() { return _impl ? _impl->exitCode() : std::nullopt; }

void ChildProcess::requestStop() {
    if (_impl) {
        _impl->requestStop();
    }
}

void ChildProcess::kill() {
    if (_impl) {
        _impl->kill();
    }
}

bool ChildProcess::waitFor(std::chrono::milliseconds timeout) {
    if (!_impl) {
        return true;
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (_impl->isRunning()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(kPollDelay);
    }
    return true;
}

void ChildProcess::stop(std::chrono::milliseconds grace) {
    if (!_impl || !_impl->isRunning()) {
        return;
    }
    _impl->requestStop();
    if (!waitFor(grace)) {
        _impl->kill();
    }
}

std::string ChildProcess::drainOutput() { return _impl ? _impl->drainOutput() : std::string{}; }

}  // namespace rtype::editor::process

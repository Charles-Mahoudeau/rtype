/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** OutputCapture
*/

#include "OutputCapture.hpp"

#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "ConsoleLog.hpp"

namespace rtype::editor::console {

namespace {
#ifdef _WIN32
constexpr int kOut = 1;
constexpr int kError = 2;
int makePipe(std::array<int, 2>& ends) { return _pipe(ends.data(), 4096, _O_BINARY); }
int duplicate(int descriptor) { return _dup(descriptor); }
int duplicateTo(int from, int to) { return _dup2(from, to); }
int closeDescriptor(int descriptor) { return _close(descriptor); }
long long readFrom(int descriptor, char* buffer, unsigned int size) { return _read(descriptor, buffer, size); }
void writeTo(int descriptor, const char* data, unsigned int size) { static_cast<void>(_write(descriptor, data, size)); }
#else
constexpr int kOut = STDOUT_FILENO;
constexpr int kError = STDERR_FILENO;
int makePipe(std::array<int, 2>& ends) { return pipe(ends.data()); }
int duplicate(int descriptor) { return dup(descriptor); }
int duplicateTo(int from, int to) { return dup2(from, to); }
int closeDescriptor(int descriptor) { return close(descriptor); }
long long readFrom(int descriptor, char* buffer, unsigned int size) { return ::read(descriptor, buffer, size); }
void writeTo(int descriptor, const char* data, unsigned int size) { static_cast<void>(write(descriptor, data, size)); }
#endif
constexpr unsigned int kChunk = 4096;  ///< Bytes read at once.
}  // namespace

OutputCapture::OutputCapture(std::shared_ptr<ConsoleLog> log) : _log{std::move(log)} {
    std::array<int, 2> ends{-1, -1};
    const bool made = makePipe(ends) == 0;
    const auto [readEnd, writeEnd] = ends;
    if (!made || readEnd < 0 || writeEnd < 0) {
        throw std::runtime_error{"OutputCapture: cannot make a pipe"};
    }
    _pipeRead = readEnd;
    _pipeWrite = writeEnd;
    std::cout.flush();
    std::cerr.flush();
    static_cast<void>(std::fflush(stdout));
    static_cast<void>(std::fflush(stderr));
    _savedOut = duplicate(kOut);
    _savedError = duplicate(kError);
    static_cast<void>(duplicateTo(_pipeWrite, kOut));
    static_cast<void>(duplicateTo(_pipeWrite, kError));
    // A pipe is fully buffered by default: lines would arrive in blocks of kilobytes.
    static_cast<void>(std::setvbuf(stdout, nullptr, _IOLBF, BUFSIZ));
    _reader = std::thread{[this] { read(); }};
}

OutputCapture::~OutputCapture() {
    std::cout.flush();
    std::cerr.flush();
    static_cast<void>(std::fflush(stdout));
    static_cast<void>(std::fflush(stderr));
    // The output goes back where it was; the last copy of the pipe's writing end closes, and the reader sees its end.
    static_cast<void>(duplicateTo(_savedOut, kOut));
    static_cast<void>(duplicateTo(_savedError, kError));
    static_cast<void>(closeDescriptor(_pipeWrite));
    if (_reader.joinable()) {
        _reader.join();
    }
    static_cast<void>(closeDescriptor(_pipeRead));
    static_cast<void>(closeDescriptor(_savedOut));
    static_cast<void>(closeDescriptor(_savedError));
}

void OutputCapture::read() {
    std::array<char, kChunk> buffer{};
    for (;;) {
        const long long count = readFrom(_pipeRead, buffer.data(), kChunk);
        if (count <= 0) {
            return;
        }
        const auto size = static_cast<unsigned int>(count);
        writeTo(_savedOut, buffer.data(), size);  // the terminal still shows everything, errors included
        _log->write(std::string_view{buffer.data(), size});
    }
}

}  // namespace rtype::editor::console

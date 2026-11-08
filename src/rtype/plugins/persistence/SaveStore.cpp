/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SaveStore
*/

#include "plugins/persistence/SaveStore.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace rtype::plugins::persistence {
namespace {
constexpr std::string_view kMagic = "RTSAVE1";
constexpr std::string_view kExtension = ".sav";
constexpr std::string_view kTemporary = ".tmp";

/// @return The 64-bit FNV-1a hash of @p data.
std::uint64_t checksum(const std::string_view data) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char byte : data) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

SaveResult failure(std::string error) { return SaveResult{.ok = false, .data = {}, .error = std::move(error)}; }

SaveResult ioFailure(const std::string_view what, const std::error_code& code) {
    return failure(std::format("io: {}: {}", what, code.message()));
}
}  // namespace

SaveStore::SaveStore(std::filesystem::path directory) : _directory{std::move(directory)} {}

bool SaveStore::isValidName(const std::string_view name) {
    constexpr std::size_t kMaxName = 64;
    return !name.empty() && name.size() <= kMaxName && name.front() != '.' &&
           std::ranges::all_of(name, [](const char c) {
               return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' ||
                      c == '-' || c == '.';
           });
}

const std::filesystem::path& SaveStore::getDirectory() const noexcept { return _directory; }

std::filesystem::path SaveStore::pathOf(const std::string_view name) const {
    return _directory / (std::string{name} + std::string{kExtension});
}

bool SaveStore::exists(const std::string_view name) const {
    if (!isValidName(name)) {
        return false;
    }
    std::error_code code;
    return std::filesystem::is_regular_file(pathOf(name), code);
}

SaveResult SaveStore::read(const std::string_view name) const {
    if (!isValidName(name)) {
        return failure("invalid_name");
    }
    std::error_code code;
    if (!std::filesystem::is_regular_file(pathOf(name), code)) {
        return failure("not_found");
    }
    std::ifstream file{pathOf(name), std::ios::binary};
    if (!file) {
        return failure("io: cannot open the file");
    }
    const std::string content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    const std::string_view contentView{content};
    // Header: "RTSAVE1\n<size>\n<hash in hex>\n", then exactly <size> bytes.
    if (!content.starts_with(kMagic) || content.size() <= kMagic.size() || content.at(kMagic.size()) != '\n') {
        return failure("corrupt");
    }
    const std::size_t sizeEnd = content.find('\n', kMagic.size() + 1);
    const std::size_t hashEnd = sizeEnd == std::string::npos ? sizeEnd : content.find('\n', sizeEnd + 1);
    if (hashEnd == std::string::npos) {
        return failure("corrupt");
    }
    const std::string_view sizeText = contentView.substr(kMagic.size() + 1, sizeEnd - kMagic.size() - 1);
    const std::string_view hashText = contentView.substr(sizeEnd + 1, hashEnd - sizeEnd - 1);
    std::size_t size = 0;
    std::uint64_t hash = 0;
    try {
        std::size_t used = 0;
        size = std::stoull(std::string{sizeText}, &used);
        if (used != sizeText.size()) {
            return failure("corrupt");
        }
        hash = std::stoull(std::string{hashText}, &used, 16);
        if (used != hashText.size()) {
            return failure("corrupt");
        }
    } catch (const std::exception&) {
        return failure("corrupt");
    }
    const std::string_view payload = contentView.substr(hashEnd + 1);
    if (payload.size() != size || checksum(payload) != hash) {
        return failure("corrupt");
    }
    return SaveResult{.ok = true, .data = std::string{payload}, .error = {}};
}

SaveResult SaveStore::write(const std::string_view name, const std::string_view data) const {
    if (!isValidName(name)) {
        return failure("invalid_name");
    }
    std::error_code code;
    std::filesystem::create_directories(_directory, code);
    if (code) {
        return ioFailure("cannot create the save directory", code);
    }
    const std::filesystem::path target = pathOf(name);
    std::filesystem::path temporary = target;
    temporary += kTemporary;
    {
        std::ofstream file{temporary, std::ios::binary | std::ios::trunc};
        if (!file) {
            return failure("io: cannot create the temporary file");
        }
        file << kMagic << '\n' << data.size() << '\n' << std::format("{:x}", checksum(data)) << '\n';
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        file.flush();
        if (!file) {
            file.close();
            std::filesystem::remove(temporary, code);
            return failure("io: cannot write the temporary file");
        }
    }
    std::filesystem::rename(temporary, target, code);
    if (code) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return ioFailure("cannot replace the save", code);
    }
    return SaveResult{.ok = true, .data = {}, .error = {}};
}

SaveResult SaveStore::remove(const std::string_view name) const {
    if (!isValidName(name)) {
        return failure("invalid_name");
    }
    std::error_code code;
    std::filesystem::remove(pathOf(name), code);
    if (code) {
        return ioFailure("cannot delete the save", code);
    }
    std::filesystem::path temporary = pathOf(name);
    temporary += kTemporary;
    std::filesystem::remove(temporary, code);  // a leftover of an interrupted write
    return SaveResult{.ok = true, .data = {}, .error = {}};
}

}  // namespace rtype::plugins::persistence

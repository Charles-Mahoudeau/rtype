/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SizeStream
*/

#include "rtype/network/bytes/SizeStream.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>

namespace rtype::network::bytes {
SizeStream::SizeStream(const std::size_t initialSize) noexcept : _size{initialSize} {}

void SizeStream::bytes(const std::span<const std::byte> bytes) noexcept {
    if (begin()) {
        _size += bytes.size();
    }
}

void SizeStream::string(const std::string& text, const std::size_t maxLength) noexcept {
    if (!begin()) {
        return;
    }
    if (text.size() > maxLength || text.size() > std::numeric_limits<std::uint8_t>::max()) {
        _valid = false;
        return;
    }
    _size += 1 + text.size();
}

void SizeStream::padTo(const std::size_t size) noexcept {
    if (!begin()) {
        return;
    }
    if (_size > size) {
        _valid = false;
        return;
    }
    _size = size;
}

void SizeStream::rest(const std::span<const std::byte> bytes) noexcept {
    if (begin()) {
        _size += bytes.size();
        _finished = true;
    }
}

bool SizeStream::isValid() const noexcept { return _valid; }

std::size_t SizeStream::size() const noexcept { return _size; }

bool SizeStream::begin() noexcept {
    if (_finished) {
        _valid = false;
    }
    return _valid;
}
}  // namespace rtype::network::bytes

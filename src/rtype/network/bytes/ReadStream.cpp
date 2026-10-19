/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReadStream
*/

#include "rtype/network/bytes/ReadStream.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <utility>

#include "rtype/network/bytes/ByteReader.hpp"

namespace rtype::network::bytes {
ReadStream::ReadStream(ByteReader& reader, const std::size_t consumed) noexcept
    : _reader{reader}, _consumed{consumed}, _initialRemaining{reader.remainingSize()} {}

void ReadStream::bytes(const std::span<std::byte> destination) noexcept {
    if (!begin()) {
        return;
    }
    const std::span<const std::byte> source = _reader.readBytes(destination.size());
    if (!_reader.isValid()) {
        _valid = false;
        return;
    }
    std::ranges::copy(source, destination.begin());
}

void ReadStream::string(std::string& text, const std::size_t maxLength) noexcept {
    if (!begin()) {
        return;
    }
    std::string read = _reader.readString();
    if (!_reader.isValid() || read.size() > maxLength) {
        _valid = false;
        return;
    }
    text = std::move(read);
}

void ReadStream::padTo(const std::size_t size) noexcept {
    if (!begin()) {
        return;
    }
    const std::size_t position = _consumed + (_initialRemaining - _reader.remainingSize());
    if (position > size) {
        _valid = false;
        return;
    }
    _reader.skip(size - position);
}

void ReadStream::rest(std::span<const std::byte>& bytes) noexcept {
    if (begin()) {
        bytes = _reader.readBytes(_reader.remainingSize());
        _finished = true;
    }
}

bool ReadStream::isValid() const noexcept { return _valid && _reader.isValid(); }

bool ReadStream::begin() noexcept {
    if (_finished) {
        _valid = false;
    }
    return isValid();
}
}  // namespace rtype::network::bytes

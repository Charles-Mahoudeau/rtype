/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WriteStream
*/

#include "rtype/network/bytes/WriteStream.hpp"

#include <cstddef>
#include <span>
#include <string>

#include "rtype/network/bytes/ByteWriter.hpp"

namespace rtype::network::bytes {
WriteStream::WriteStream(ByteWriter& writer) noexcept : _writer{writer} {}

void WriteStream::bytes(const std::span<const std::byte> bytes) noexcept {
    if (begin()) {
        _writer.writeBytes(bytes);
    }
}

void WriteStream::string(const std::string& text, const std::size_t maxLength) noexcept {
    if (!begin()) {
        return;
    }
    if (text.size() > maxLength) {
        _valid = false;
        return;
    }
    _writer.writeString(text);
}

void WriteStream::padTo(const std::size_t size) noexcept {
    if (!begin()) {
        return;
    }
    const std::size_t written = _writer.bytesWritten();
    if (written > size) {
        _valid = false;
        return;
    }
    _writer.writeZeros(size - written);
}

void WriteStream::rest(const std::span<const std::byte> bytes) noexcept {
    if (begin()) {
        _writer.writeBytes(bytes);
        _finished = true;
    }
}

bool WriteStream::isValid() const noexcept { return _valid && _writer.isValid(); }

bool WriteStream::begin() noexcept {
    if (_finished) {
        _valid = false;
    }
    return isValid();
}
}  // namespace rtype::network::bytes

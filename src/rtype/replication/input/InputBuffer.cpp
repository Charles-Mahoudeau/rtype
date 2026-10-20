/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputBuffer
*/

#include "rtype/replication/input/InputBuffer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "rtype/replication/input/InputMessage.hpp"

namespace rtype::replication::input {
std::uint32_t InputBuffer::lastProcessed() const noexcept { return _lastProcessed; }

void InputBuffer::push(const InputSample& sample) {
    if (_hasProcessed && sample.sequence <= _lastProcessed) {
        return;
    }
    const auto position = std::ranges::lower_bound(_pending, sample.sequence, {}, &InputSample::sequence);
    if (position != _pending.end() && position->sequence == sample.sequence) {
        return;
    }
    _pending.insert(position, sample);
    if (_pending.size() > kMaxBuffered) {
        _pending.erase(_pending.begin(), _pending.end() - static_cast<std::ptrdiff_t>(kKeptOnOverflow));
    }
}

const InputSample& InputBuffer::next() {
    if (!_pending.empty()) {
        _current = _pending.front();
        _pending.erase(_pending.begin());
        _lastProcessed = _current.sequence;
        _hasProcessed = true;
    }
    return _current;
}

void InputBuffer::reset() noexcept {
    _pending.clear();
    _current = InputSample{};
    _lastProcessed = 0;
    _hasProcessed = false;
}
}  // namespace rtype::replication::input

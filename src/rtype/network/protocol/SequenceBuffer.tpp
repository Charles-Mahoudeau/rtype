/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SequenceBuffer
*/

#pragma once

namespace rtype::network::protocol {
template <typename T, std::size_t N>
T& SequenceBuffer<T, N>::insert(std::uint16_t sequence) {
    Entry& entry = _entries[slotOf(sequence)];
    entry.value = T{};
    entry.sequence = sequence;
    entry.occupied = true;
    return entry.value;
}

template <typename T, std::size_t N>
T* SequenceBuffer<T, N>::find(std::uint16_t sequence) noexcept {
    Entry& entry = _entries[slotOf(sequence)];
    return entry.occupied && entry.sequence == sequence ? &entry.value : nullptr;
}

template <typename T, std::size_t N>
const T* SequenceBuffer<T, N>::find(std::uint16_t sequence) const noexcept {
    const Entry& entry = _entries[slotOf(sequence)];
    return entry.occupied && entry.sequence == sequence ? &entry.value : nullptr;
}

template <typename T, std::size_t N>
void SequenceBuffer<T, N>::remove(std::uint16_t sequence) noexcept {
    Entry& entry = _entries[slotOf(sequence)];
    if (entry.occupied && entry.sequence == sequence) {
        entry.occupied = false;
    }
}
}  // namespace rtype::network::protocol

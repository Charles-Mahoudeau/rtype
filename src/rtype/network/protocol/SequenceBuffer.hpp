/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SequenceBuffer
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rtype::network::protocol {
/// @brief Data about each of the last N sequence numbers, in a fixed array: number `s` lives in slot `s % N`.
///
/// @details Nothing is allocated after construction and every access is constant-time. A slot holds one value,
/// tagged with the number that wrote it, so an older number is forgotten as soon as a newer one takes its slot. The
/// buffer never compares sequences and never decides what is too old: that is the caller's job, with `isNewer`.
/// @tparam T The data kept per number; it must be default-constructible and assignable.
/// @tparam N The number of slots; a power of two, so that 65536 is a multiple of it and two numbers fewer than N
/// apart never share a slot across the wrap.
template <typename T, std::size_t N>
class SequenceBuffer {
    static_assert(N > 0 && (N & (N - 1)) == 0, "SequenceBuffer: N must be a power of two");

  public:
    /// @brief Builds an empty buffer: `find` returns nullptr for every number, including 0.
    SequenceBuffer() = default;

    /// @brief Destroys the buffer and the values it holds.
    ~SequenceBuffer() = default;

    /// @brief Copies every slot.
    SequenceBuffer(const SequenceBuffer&) = default;

    /// @brief Copy-assigns every slot.
    SequenceBuffer& operator=(const SequenceBuffer&) = default;

    /// @brief Moves every slot.
    SequenceBuffer(SequenceBuffer&&) noexcept = default;

    /// @brief Move-assigns every slot.
    SequenceBuffer& operator=(SequenceBuffer&&) noexcept = default;

    /// @brief Takes the slot of `sequence`, forgetting what was there, and returns its value reset to `T{}`.
    /// @param sequence The number to store data for.
    /// @return A reference to the value, valid until the slot is taken again or the buffer is destroyed.
    T& insert(std::uint16_t sequence);

    /// @brief Looks up the value stored for `sequence`.
    /// @param sequence The number to look for.
    /// @return The value, or nullptr if its slot is empty or holds another number.
    [[nodiscard]] T* find(std::uint16_t sequence) noexcept;

    /// @brief Looks up the value stored for `sequence`.
    /// @param sequence The number to look for.
    /// @return The value, or nullptr if its slot is empty or holds another number.
    [[nodiscard]] const T* find(std::uint16_t sequence) const noexcept;

    /// @brief Empties the slot of `sequence` if it still holds `sequence`; a newer number in the slot is kept.
    /// @param sequence The number to forget.
    void remove(std::uint16_t sequence) noexcept;

  private:
    /// @brief One slot: a value and the number that wrote it.
    struct Entry {
        T value{};                  ///< The data kept for the number.
        std::uint16_t sequence{0};  ///< The number that wrote this slot.
        bool occupied{false};       ///< False until the first insert, and after a remove.
    };

    [[nodiscard]] static constexpr std::size_t slotOf(const std::uint16_t sequence) noexcept { return sequence % N; }

    std::array<Entry, N> _entries{};  ///< Slot `s % N` for sequence `s`.
};
}  // namespace rtype::network::protocol

#include "SequenceBuffer.tpp"  // IWYU pragma: export

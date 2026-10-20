/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputBuffer
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "rtype/replication/Export.hpp"
#include "rtype/replication/input/InputMessage.hpp"

namespace rtype::replication::input {
/// @brief The inputs of one client on the server: orders them by sequence and hands out one per tick.
///
/// @details Datagrams arrive in bursts, late or twice; the game wants exactly one input per tick, in order, each at
/// most once. When none arrived, the last input is repeated.
class RTYPE_REPLICATION_API InputBuffer {
  public:
    static constexpr std::size_t kMaxBuffered = 4;     ///< Above this after a push, the oldest are dropped.
    static constexpr std::size_t kKeptOnOverflow = 2;  ///< How many remain after an overflow.

    InputBuffer() = default;
    ~InputBuffer() = default;
    InputBuffer(const InputBuffer& other) = default;
    InputBuffer& operator=(const InputBuffer& other) = default;
    InputBuffer(InputBuffer&& other) noexcept = default;
    InputBuffer& operator=(InputBuffer&& other) noexcept = default;

    /// @return The sequence of the last input handed out by next(), 0 before any.
    [[nodiscard]] std::uint32_t lastProcessed() const noexcept;

    /// @brief Adds an input in sequence order; ignores a sequence already applied or already buffered. If more than
    /// kMaxBuffered remain, drops the oldest until kKeptOnOverflow remain; they are never applied.
    void push(const InputSample& sample);

    /// @brief Hands out the lowest buffered input and makes it the current one. With nothing buffered, returns the
    /// current one again and the last processed input does not move; before any input it is neutral.
    /// @return The input to apply this tick, valid until the next call.
    [[nodiscard]] const InputSample& next();

    /// @brief Empties the buffer and makes the current input neutral.
    void reset() noexcept;

  private:
    std::vector<InputSample> _pending;  ///< Ordered by sequence, lowest first.
    InputSample _current;               ///< The input handed out last.
    std::uint32_t _lastProcessed{0};    ///< The sequence of _current.
    bool _hasProcessed{false};          ///< True once next() handed out an input.
};
}  // namespace rtype::replication::input

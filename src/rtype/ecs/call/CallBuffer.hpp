/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallBuffer
*/

#pragma once

#include <cstddef>
#include <new>
#include <optional>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {

/// @brief The bytes of one value of a call data type (CallInfo::arguments or CallInfo::result), as Calls::invoke()
/// expects them: aligned as the type requires and holding a value built from its defaults, destroyed with the buffer.
///
/// @details Plain call data could live in any bytes, but call data with a text or a list (a `std::string` field) is an
/// object that owns memory: it must be constructed where the call reads it, and destroyed once. A script runtime makes
/// one buffer for the arguments, fills the fields the script names, makes one for the result, and invokes the call.
/// @code
/// rtype::ecs::CallBuffer arguments{info.arguments};
/// rtype::ecs::CallBuffer result{info.result};
/// world.getCalls().invoke(world, info.id, arguments.get(), result.get());
/// @endcode
class RTYPE_ECS_API CallBuffer {
  public:
    /// @brief Builds a value of @p type from its defaults, or nothing when there is no type or it is empty.
    explicit CallBuffer(const std::optional<TypeInfo>& type);
    ~CallBuffer();
    CallBuffer(const CallBuffer& other) = delete;
    CallBuffer& operator=(const CallBuffer& other) = delete;
    CallBuffer(CallBuffer&& other) = delete;
    CallBuffer& operator=(CallBuffer&& other) = delete;

    /// @return The bytes of the value; empty when there is none.
    [[nodiscard]] std::span<std::byte> get() const noexcept { return {_data, _size}; }

  private:
    std::byte* _data{nullptr};                               ///< The value, or null.
    std::size_t _size{0};                                    ///< Its size in bytes.
    std::align_val_t _alignment{alignof(std::max_align_t)};  ///< Its alignment, for the delete.
    void (*_destruct)(void* target) noexcept = nullptr;      ///< Destroys the value; null when it is plain data.
};

}  // namespace rtype::ecs

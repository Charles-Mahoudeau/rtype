/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Exceptions
*/

#pragma once

#include <stdexcept>

#include "Export.hpp"

namespace rtype::luau::exceptions {
/// @brief Base class of every exception thrown by the Luau module.
///
/// Catching it catches any error raised by the module; it can also be caught as a `std::runtime_error`.
class RTYPE_LUAU_API Base : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

/// @brief Exception thrown when a null Lua state is given where a valid one is required.
class RTYPE_LUAU_API InvalidState : public Base {
  public:
    /// @brief Creates the exception with the message "invalid Lua state".
    InvalidState() : Base{"invalid Lua state"} {}
};

/// @brief Exception thrown when an empty Ref (default-constructed or moved-from) is used.
class RTYPE_LUAU_API InvalidRef : public Base {
  public:
    /// @brief Creates the exception with the message "use of an empty Luau reference".
    InvalidRef() : Base{"use of an empty Luau reference"} {}
};

/// @brief Exception thrown when an operation is not supported by the object it is called on.
///
/// For instance, binding a native closure to a Function that wraps a function defined in Luau.
class RTYPE_LUAU_API InvalidOperation : public Base {
  public:
    using Base::Base;
};

/// @brief Exception thrown when Luau code called from C++ raises an error.
///
/// Its message is the error message raised by the Luau code.
class RTYPE_LUAU_API RuntimeError : public Base {
  public:
    using Base::Base;
};
}  // namespace rtype::luau::exceptions

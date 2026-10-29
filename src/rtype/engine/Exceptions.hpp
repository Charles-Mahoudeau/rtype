/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Exceptions
*/

#pragma once

#include <stdexcept>

#include "Export.hpp"

namespace rtype::engine::exceptions {
/// @brief Exception thrown when the engine fails while running, for instance when a script cannot be loaded.
///
/// It can be caught as a `std::runtime_error`.
class RTYPE_ENGINE_API RuntimeError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};
}  // namespace rtype::engine::exceptions

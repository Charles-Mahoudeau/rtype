/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a query given to a system cannot be built: a component that is not registered, or
/// terms that contradict each other. The message names the query and the component involved.
/// @details Exported, with its destructor defined in the library: it is thrown inside the library and caught in the
/// game, so its type information must be one, shared object (macOS compares exception types by address).
class RTYPE_ECS_API QueryException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the query and the component.
    explicit QueryException(const std::string& message);
    ~QueryException() override;
    QueryException(const QueryException& other) = default;
    QueryException& operator=(const QueryException& other) = default;
    QueryException(QueryException&& other) noexcept = default;
    QueryException& operator=(QueryException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions

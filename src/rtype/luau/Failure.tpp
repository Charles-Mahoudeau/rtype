/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Failure
*/

#include <tl/expected.hpp>
#include <utility>

namespace rtype::luau {
template <typename T>
Failure::operator Result<T>() && noexcept {
    return tl::unexpected{std::move(_error)};
}
}  // namespace rtype::luau

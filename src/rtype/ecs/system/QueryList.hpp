/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryList
*/

#pragma once

#include <vector>

#include "rtype/ecs/query/QueryBuilder.hpp"

namespace rtype::ecs {
/// @brief The configuration of a DynamicQueries parameter: the queries the system runs, given when it is added.
/// @details Each builder becomes one query, in the same order (DynamicQueries::iter(0) is the first). The components
/// they name must be registered before the system is added.
/// @code
/// app.addSystem(rtype::ecs::Phase::kPostUpdate, "net.snapshot", &snapshot,
///               rtype::ecs::QueryList{{rtype::ecs::QueryBuilder{}.read(networkId).read(health)}});
/// @endcode
struct QueryList {
    std::vector<QueryBuilder> queries;  ///< One builder per query, in order.
};
}  // namespace rtype::ecs

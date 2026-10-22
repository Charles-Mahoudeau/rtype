/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryFilters
*/

#include "plugins/physics/detail/QueryFilters.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdint>

#include "plugins/physics/detail/Layers.hpp"

namespace rtype::plugins::physics::detail {

MaskLayerFilter::MaskLayerFilter(std::uint32_t mask) : _mask{mask} {}

bool MaskLayerFilter::ShouldCollide(JPH::ObjectLayer layer) const {
    return (layers::categoriesOf(layer) & _mask) != 0U;
}

QueryBodyFilter::QueryBodyFilter(JPH::BodyID ignored, bool includeSensors)
    : _ignored{ignored}, _includeSensors{includeSensors} {}

bool QueryBodyFilter::ShouldCollide(const JPH::BodyID& body) const { return body != _ignored; }

bool QueryBodyFilter::ShouldCollideLocked(const JPH::Body& body) const {
    return (_includeSensors || !body.IsSensor()) && body.GetID() != _ignored;
}

}  // namespace rtype::plugins::physics::detail

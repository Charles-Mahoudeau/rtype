/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Layers
*/

#include "plugins/physics/detail/Layers.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdint>

namespace rtype::plugins::physics::detail {

namespace layers {

bool isValid(std::uint32_t categories, std::uint32_t mask) noexcept {
    return categories != 0U && (categories & ~kAllCategories) == 0U && (mask & ~kAllCategories) == 0U;
}

JPH::ObjectLayer encode(std::uint32_t categories, std::uint32_t mask, bool isStatic) noexcept {
    return static_cast<JPH::ObjectLayer>((categories & kAllCategories) | ((mask & kAllCategories) << kCategoryCount) |
                                         (isStatic ? kStaticBit : 0U));
}

std::uint32_t categoriesOf(JPH::ObjectLayer layer) noexcept {
    return static_cast<std::uint32_t>(layer) & kAllCategories;
}

std::uint32_t maskOf(JPH::ObjectLayer layer) noexcept {
    return (static_cast<std::uint32_t>(layer) >> kCategoryCount) & kAllCategories;
}

bool isStatic(JPH::ObjectLayer layer) noexcept { return (static_cast<std::uint32_t>(layer) & kStaticBit) != 0U; }

bool canCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) noexcept {
    return !(isStatic(first) && isStatic(second)) && (categoriesOf(first) & maskOf(second)) != 0U &&
           (categoriesOf(second) & maskOf(first)) != 0U;
}

}  // namespace layers

JPH::uint BroadPhaseLayers::GetNumBroadPhaseLayers() const { return broadPhaseLayers::kCount; }

JPH::BroadPhaseLayer BroadPhaseLayers::GetBroadPhaseLayer(JPH::ObjectLayer layer) const {
    return layers::isStatic(layer) ? broadPhaseLayers::kNonMoving : broadPhaseLayers::kMoving;
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* BroadPhaseLayers::GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const {
    return layer == broadPhaseLayers::kNonMoving ? "NonMoving" : "Moving";
}
#endif

bool ObjectVsBroadPhaseFilter::ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const {
    return !layers::isStatic(layer) || broadPhaseLayer == broadPhaseLayers::kMoving;
}

bool ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const {
    return layers::canCollide(first, second);
}

}  // namespace rtype::plugins::physics::detail

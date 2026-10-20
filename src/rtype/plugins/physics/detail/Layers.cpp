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

namespace rtype::plugins::physics::detail {

JPH::uint BroadPhaseLayers::GetNumBroadPhaseLayers() const { return broadPhaseLayers::kCount; }

JPH::BroadPhaseLayer BroadPhaseLayers::GetBroadPhaseLayer(JPH::ObjectLayer layer) const {
    return layer == layers::kNonMoving ? broadPhaseLayers::kNonMoving : broadPhaseLayers::kMoving;
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* BroadPhaseLayers::GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const {
    return layer == broadPhaseLayers::kNonMoving ? "NonMoving" : "Moving";
}
#endif

bool ObjectVsBroadPhaseFilter::ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const {
    return layer != layers::kNonMoving || broadPhaseLayer == broadPhaseLayers::kMoving;
}

bool ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const {
    return first != layers::kNonMoving || second != layers::kNonMoving;
}

}  // namespace rtype::plugins::physics::detail

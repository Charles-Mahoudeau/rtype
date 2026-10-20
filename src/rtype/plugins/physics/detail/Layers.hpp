/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Layers
*/

#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdint>

namespace rtype::plugins::physics::detail {

/// @brief The object layers, until collision layers are configurable: static bodies never collide with each other.
namespace layers {
inline constexpr JPH::ObjectLayer kNonMoving = 0;  ///< Static bodies.
inline constexpr JPH::ObjectLayer kMoving = 1;     ///< Dynamic and kinematic bodies.
inline constexpr std::uint32_t kCount = 2;         ///< Object layers in use.
}  // namespace layers

/// @brief The broad phase layers: one tree for static bodies, one for moving ones.
namespace broadPhaseLayers {
inline constexpr JPH::BroadPhaseLayer kNonMoving{0};  ///< Static bodies.
inline constexpr JPH::BroadPhaseLayer kMoving{1};     ///< Dynamic and kinematic bodies.
inline constexpr std::uint32_t kCount = 2;            ///< Broad phase layers in use.
}  // namespace broadPhaseLayers

/// @brief Maps each object layer to its broad phase layer.
class BroadPhaseLayers final : public JPH::BroadPhaseLayerInterface {
  public:
    BroadPhaseLayers() = default;
    ~BroadPhaseLayers() override = default;
    BroadPhaseLayers(const BroadPhaseLayers& other) = delete;
    BroadPhaseLayers& operator=(const BroadPhaseLayers& other) = delete;
    BroadPhaseLayers(BroadPhaseLayers&& other) noexcept = delete;
    BroadPhaseLayers& operator=(BroadPhaseLayers&& other) noexcept = delete;

    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override;
    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override;
#endif
};

/// @brief Whether an object layer can collide with a broad phase layer: static bodies skip the static tree.
class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
  public:
    ObjectVsBroadPhaseFilter() = default;
    ~ObjectVsBroadPhaseFilter() override = default;
    ObjectVsBroadPhaseFilter(const ObjectVsBroadPhaseFilter& other) = delete;
    ObjectVsBroadPhaseFilter& operator=(const ObjectVsBroadPhaseFilter& other) = delete;
    ObjectVsBroadPhaseFilter(ObjectVsBroadPhaseFilter&& other) noexcept = delete;
    ObjectVsBroadPhaseFilter& operator=(ObjectVsBroadPhaseFilter&& other) noexcept = delete;

    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override;
};

/// @brief Whether two object layers can collide: everything but two static bodies.
class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
  public:
    ObjectLayerPairFilter() = default;
    ~ObjectLayerPairFilter() override = default;
    ObjectLayerPairFilter(const ObjectLayerPairFilter& other) = delete;
    ObjectLayerPairFilter& operator=(const ObjectLayerPairFilter& other) = delete;
    ObjectLayerPairFilter(ObjectLayerPairFilter&& other) noexcept = delete;
    ObjectLayerPairFilter& operator=(ObjectLayerPairFilter&& other) noexcept = delete;

    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override;
};

}  // namespace rtype::plugins::physics::detail

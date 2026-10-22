/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryFilters
*/
#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <cstdint>

namespace rtype::plugins::physics::detail {

/// @brief Accepts the bodies whose collision categories are in the query's mask (QueryFilter::mask).
class MaskLayerFilter final : public JPH::ObjectLayerFilter {
  public:
    explicit MaskLayerFilter(std::uint32_t mask);
    ~MaskLayerFilter() override = default;
    MaskLayerFilter(const MaskLayerFilter& other) = delete;
    MaskLayerFilter& operator=(const MaskLayerFilter& other) = delete;
    MaskLayerFilter(MaskLayerFilter&& other) noexcept = delete;
    MaskLayerFilter& operator=(MaskLayerFilter&& other) noexcept = delete;

    /// @return Whether a body in this object layer is seen: its categories meet the mask (layers::categoriesOf).
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer layer) const override;

  private:
    std::uint32_t _mask;  ///< The categories the query sees.
};

/// @brief Skips the ignored body (QueryFilter::ignore) and, unless asked, sensors (QueryFilter::includeSensors).
class QueryBodyFilter final : public JPH::BodyFilter {
  public:
    QueryBodyFilter(JPH::BodyID ignored, bool includeSensors);
    ~QueryBodyFilter() override = default;
    QueryBodyFilter(const QueryBodyFilter& other) = delete;
    QueryBodyFilter& operator=(const QueryBodyFilter& other) = delete;
    QueryBodyFilter(QueryBodyFilter&& other) noexcept = delete;
    QueryBodyFilter& operator=(QueryBodyFilter&& other) noexcept = delete;
    /// @return False for the ignored body, without locking it.
    [[nodiscard]] bool ShouldCollide(const JPH::BodyID& body) const override;
    /// @return False for a sensor, unless sensors are included (the body is locked here, so it can be read).
    [[nodiscard]] bool ShouldCollideLocked(const JPH::Body& body) const override;

  private:
    JPH::BodyID _ignored;  ///< Invalid when nothing is ignored.
    bool _includeSensors;  ///< Whether sensors are hit.
};

}  // namespace rtype::plugins::physics::detail

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::transform {

/// @brief Positions entities: registers Transform and GlobalTransform, and computes GlobalTransform from Transform.
///
/// @details Two PostUpdate systems, on every side (Side::kBoth: the server's gameplay and the client's rendering need
/// them):
/// - "rtype.Transform.Insert": gives a GlobalTransform, equal to its Transform, to every entity that has a Transform
///   but no GlobalTransform yet (spawned this frame); the commands apply at the end of PostUpdate, so Render sees it.
/// - "rtype.Transform.Propagate", after it: copies each changed Transform into its GlobalTransform.
/// Systems that move entities write Transform before PostUpdate; systems that read positions (rendering) read
/// GlobalTransform after it.
class TransformPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Transform";                       ///< The plugin's name.
    static constexpr std::string_view kInsertSystem = "rtype.Transform.Insert";        ///< Adds GlobalTransform.
    static constexpr std::string_view kPropagateSystem = "rtype.Transform.Propagate";  ///< Updates GlobalTransform.

    TransformPlugin() = default;
    ~TransformPlugin() override = default;
    TransformPlugin(const TransformPlugin& other) = default;
    TransformPlugin& operator=(const TransformPlugin& other) = default;
    TransformPlugin(TransformPlugin&& other) noexcept = default;
    TransformPlugin& operator=(TransformPlugin&& other) noexcept = default;

    /// @return "rtype.Transform".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers the components and adds the PostUpdate systems.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::transform

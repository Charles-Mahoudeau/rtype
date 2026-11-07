/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldEditor
*/

#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "editor/Export.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::editor::panels {

/// @brief The ImGui drag-and-drop payload of a file of the assets: its path in them, as text without a terminator.
inline constexpr const char* kAssetPayload = "RTYPE_ASSET";
/// @brief The ImGui drag-and-drop payload of a node of the scene: its index, a std::uint32_t.
inline constexpr const char* kNodePayload = "RTYPE_NODE";

/// @brief A value of a component or a resource, built as its type builds it by default and destroyed with this: what
/// "Add component" adds.
class RTYPE_EDITOR_API DefaultValue {
  public:
    /// @pre @p type's alignment is at most alignof(std::max_align_t).
    explicit DefaultValue(const ecs::TypeInfo& type);
    ~DefaultValue();
    DefaultValue(const DefaultValue& other) = delete;
    DefaultValue& operator=(const DefaultValue& other) = delete;
    DefaultValue(DefaultValue&& other) = delete;
    DefaultValue& operator=(DefaultValue&& other) = delete;

    /// @return The value's bytes, the type's size.
    [[nodiscard]] std::span<const std::byte> getBytes() const noexcept;

  private:
    const ecs::TypeInfo* _type;              ///< The value's type.
    std::vector<std::max_align_t> _storage;  ///< Aligned for any field type; the value lives at its start.
};

/// @return Whether @p name is a component the editor never shows, adds or removes: the hierarchy (Parent, Children,
/// set by the tree), what a capsule's spawn sets (CapsuleInstance) and, in a scene, what the node itself gives
/// (Name, its name) or the engine computes (GlobalTransform).
[[nodiscard]] RTYPE_EDITOR_API bool isHiddenComponent(std::string_view name, bool inScene) noexcept;

/// @brief Shows the fields of a value with the widget of their type, and lets the user change them in place.
/// @details @p value is a copy of the value's bytes that the caller writes back when this returns true: copied byte for
/// byte, it shares what the value owns (a list's elements) and must not be destroyed, and only plain fields are
/// changed. An asset field shows the path the capsules know it by, takes a file dropped from the Assets panel, and an
/// image dropped there is loaded as a texture (TextureAssets) so it shows at once.
/// @return Whether a field changed.
RTYPE_EDITOR_API bool editFields(ecs::World& world, const ecs::TypeInfo& type, std::span<std::byte> value);

}  // namespace rtype::editor::panels

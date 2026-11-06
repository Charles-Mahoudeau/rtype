/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentValues
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"

namespace rtype::ecs {
/// @brief Component values prepared once and copied many times: what a prefab gives each entity it spawns.
/// @details One value per component, kept in the order they were first set, each a live value managed with its type's
/// hooks (a C++ component holding a `std::vector` is copied and destroyed properly). Setting a component again
/// replaces its value. Every component is copyable (validate() refuses one that is not), so any of them can be set.
/// @code
/// rtype::ecs::ComponentValues values;
/// values.set(world.getComponents(), Health{.value = 3});
/// values.set(world.getComponents(), positionId, positionBytes);  // by id and bytes: the Luau bridge
/// @endcode
class RTYPE_ECS_API ComponentValues {
  public:
    ComponentValues() = default;
    ~ComponentValues() = default;
    ComponentValues(const ComponentValues& other) = delete;
    ComponentValues& operator=(const ComponentValues& other) = delete;
    ComponentValues(ComponentValues&& other) noexcept = default;
    ComponentValues& operator=(ComponentValues&& other) noexcept = default;

    /// @return The number of components that have a value.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The component of the value at `index`, in the order they were first set; `index` below getCount().
    [[nodiscard]] ComponentId getId(std::size_t index) const noexcept;

    /// @return The bytes of the value at `index`, empty for a tag; `index` below getCount().
    [[nodiscard]] std::span<const std::byte> getValue(std::size_t index) const noexcept;

    /// @return Whether the component has a value.
    [[nodiscard]] bool has(ComponentId id) const noexcept;

    /// @return The bytes of the component's value; empty if it has none, and for a tag (use has()).
    [[nodiscard]] std::span<const std::byte> get(ComponentId id) const noexcept;

    /// @return The C++ component's value, or nullptr if it has none or `T` is not registered.
    template <Component T>
    [[nodiscard]] const T* get(const Components& components) const noexcept;

    /// @brief Sets the component to a copy of a value, replacing its value if it has one.
    /// @param value The bytes of a live value of the component, exactly its size (empty for a tag).
    /// @throws exceptions::PrefabException When the id is not a registered component, or `value` has the wrong size.
    void set(const Components& components, ComponentId id, std::span<const std::byte> value);

    /// @brief Sets the C++ component to a value, moved in, replacing its value if it has one.
    /// @details Registers `T` with the default options if it is not registered yet, as Components::add() does.
    template <Component T>
    void set(Components& components, T value);

    /// @brief Removes the component's value.
    /// @return False, doing nothing, if it has none.
    bool remove(ComponentId id) noexcept;

  private:
    /// @brief One component's value.
    struct Entry {
        ComponentId id;              ///< The component.
        storage::ByteColumn column;  ///< Its value, the column's only element.
    };

    /// @return The entry of the component, or nullptr.
    [[nodiscard]] const Entry* find(ComponentId id) const noexcept;

    /// @return An empty column for one value of the component.
    [[nodiscard]] static storage::ByteColumn makeColumn(const ComponentInfo& info);

    /// @brief Makes `column` (holding the new value) the component's, replacing its old value if it had one.
    void store(ComponentId id, storage::ByteColumn column);

    std::vector<Entry> _entries;  ///< The values, in the order the components were first set.
};
}  // namespace rtype::ecs

#define RTYPE_COMPONENT_VALUES_INCLUDING_TPP
#include "ComponentValues.tpp"  // IWYU pragma: export
#undef RTYPE_COMPONENT_VALUES_INCLUDING_TPP

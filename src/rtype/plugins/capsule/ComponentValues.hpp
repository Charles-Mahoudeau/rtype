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

#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::plugins::capsule {
/// @brief Component values prepared once and copied many times: what a capsule node gives the entity it spawns.
/// @details One value per component, kept in the order they were first set, each a live value managed with its type's
/// hooks (a C++ component holding a `std::vector` is copied and destroyed properly). Setting a component again
/// replaces its value. Every component is copyable (`ecs::validate()` refuses one that is not), so any of them can be
/// set. Each value also keeps its component's Presence, read when it is set, so a spawn needs no registry.
/// @code
/// rtype::plugins::capsule::ComponentValues values;
/// values.set(world.getComponents(), Health{.value = 3});
/// values.set(world.getComponents(), positionId, positionBytes);  // by id and bytes: the Luau bridge
/// @endcode
class RTYPE_PLUGIN_CAPSULE_API ComponentValues {
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
    [[nodiscard]] ecs::ComponentId getId(std::size_t index) const noexcept;

    /// @return The bytes of the value at `index`, empty for a tag; `index` below getCount().
    [[nodiscard]] std::span<const std::byte> getValue(std::size_t index) const noexcept;

    /// @return On which side of the network the component of the value at `index` exists; `index` below getCount().
    [[nodiscard]] ecs::Presence getPresence(std::size_t index) const noexcept;

    /// @return Whether the component has a value.
    [[nodiscard]] bool has(ecs::ComponentId id) const noexcept;

    /// @return The bytes of the component's value; empty if it has none, and for a tag (use has()).
    [[nodiscard]] std::span<const std::byte> get(ecs::ComponentId id) const noexcept;

    /// @return The C++ component's value, or nullptr if it has none or `T` is not registered.
    template <ecs::Component T>
    [[nodiscard]] const T* get(const ecs::Components& components) const noexcept;

    /// @brief Sets the component to a copy of a value, replacing its value if it has one.
    /// @param value The bytes of a live value of the component, exactly its size (empty for a tag).
    /// @throws exceptions::CapsuleException When the id is not a registered component, is Parent or Children (an
    /// capsule's nodes set the hierarchy), or `value` has the wrong size.
    void set(const ecs::Components& components, ecs::ComponentId id, std::span<const std::byte> value);

    /// @brief Sets the C++ component to a value, moved in, replacing its value if it has one.
    /// @details Registers `T` with the default options if it is not registered yet, as `ecs::Components::add()` does.
    /// Parent and Children are refused at compile time: a capsule's nodes set the hierarchy.
    template <ecs::Component T>
    void set(ecs::Components& components, T value);

    /// @brief Removes the component's value.
    /// @return False, doing nothing, if it has none.
    bool remove(ecs::ComponentId id) noexcept;

  private:
    /// @brief One component's value.
    struct Entry {
        ecs::ComponentId id;              ///< The component.
        ecs::Presence presence;           ///< On which side of the network it exists.
        ecs::storage::ByteColumn column;  ///< Its value, the column's only element.
    };

    /// @return The entry of the component, or nullptr.
    [[nodiscard]] const Entry* find(ecs::ComponentId id) const noexcept;

    /// @return An empty column for one value of the component.
    [[nodiscard]] static ecs::storage::ByteColumn makeColumn(const ecs::ComponentInfo& info);

    /// @brief Makes `column` (holding the new value) the component's, replacing its old value if it had one.
    void store(const ecs::ComponentInfo& info, ecs::storage::ByteColumn column);

    std::vector<Entry> _entries;  ///< The values, in the order the components were first set.
};
}  // namespace rtype::plugins::capsule

#define RTYPE_COMPONENT_VALUES_INCLUDING_TPP
#include "ComponentValues.tpp"  // IWYU pragma: export
#undef RTYPE_COMPONENT_VALUES_INCLUDING_TPP

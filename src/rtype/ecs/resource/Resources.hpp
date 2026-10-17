/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Resources
*/

#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <span>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"

namespace rtype::ecs {
namespace detail {
struct WorldState;
}  // namespace detail

/// @brief The resources of a world: their registry and one slot per resource, each holding at most one value owned
/// by the world (the time, the input, the renderer).
///
/// @details Reached through World::getResources(); it only exists inside a world, whose change tick it reads.
/// Resource ids are per world, and only names are shared across worlds and processes.
/// @code
/// rtype::ecs::Resources& resources = world.getResources();
/// resources.insert(Time{.delta = 0.F, .fixedDelta = 1.F / 60.F});
/// const Time& time = resources.require<Time>();
/// @endcode
class RTYPE_ECS_API Resources {
  public:
    /// @param state The state of the world these resources belong to; it must outlive them.
    explicit Resources(const detail::WorldState& state) noexcept;
    /// @brief Destroys every resource value.
    ~Resources() = default;
    Resources(const Resources& other) = delete;
    Resources& operator=(const Resources& other) = delete;
    Resources(Resources&& other) = delete;
    Resources& operator=(Resources&& other) = delete;

    /// @return The resources registered in this world.
    [[nodiscard]] const ResourceRegistry& getRegistry() const noexcept;

    /// @brief Registers a C++ resource, or returns its id if it is already registered with the same options.
    /// @details insert() and insertNonSend() register the resource themselves; call this to know its id, or to
    /// register it before its value exists.
    /// @throws exceptions::ResourceRegistrationException When the name is taken by another layout, by a component or
    /// an event, or is registered with the other main-thread flag.
    template <Resource T>
    ResourceId registerType(ResourceOptions options = {});

    /// @brief Registers a resource described at run time (a Luau resource), and creates its empty slot.
    /// @return Its id (the existing one if the same description was registered before), or why it was refused.
    [[nodiscard]] Result<ResourceId> registerType(ResourceInfo info);

    /// @return The id of the resource with this name in this world, or nothing if it is not registered.
    [[nodiscard]] std::optional<ResourceId> find(std::string_view name) const noexcept;

    /// @brief Sets the resource to a copy of a value, replacing the current one if any.
    /// @details Used by the Luau bridge and the network, which only know resources by id. Stamps the world's change
    /// tick (only as the changed tick when replacing). If copying throws, the current value is kept.
    /// @param value The bytes of a live value of the resource, exactly its size (empty for an empty type).
    /// @return False, doing nothing, if the id is unknown, `value` has the wrong size, or the resource is not
    /// copyable.
    bool insert(ResourceId id, std::span<const std::byte> value);

    /// @return Whether the resource holds a value; false for an unknown id.
    [[nodiscard]] bool has(ResourceId id) const noexcept;

    /// @return The bytes of the resource's value; empty if it has none or the id is unknown, and always for an empty
    /// type (use has()). Invalidated by the next insertion or removal of the resource.
    [[nodiscard]] std::span<const std::byte> get(ResourceId id) const noexcept;

    /// @brief Write access to the resource's value, marked changed now: the caller only has bytes, so it cannot tell
    /// a read from a write. Use get() to only read.
    /// @return The bytes of the value; empty if it has none or the id is unknown, and always for an empty type.
    [[nodiscard]] std::span<std::byte> getMut(ResourceId id) noexcept;

    /// @brief Destroys the resource's value. The resource stays registered.
    /// @return False, doing nothing, if it has no value or the id is unknown.
    bool remove(ResourceId id) noexcept;

    /// @return When the resource's value was inserted and last written, or nothing if it has no value or the id is
    /// unknown.
    [[nodiscard]] std::optional<ComponentTicks> getTicks(ResourceId id) const noexcept;

    /// @brief Sets a C++ resource, replacing its value if it already has one.
    /// @details Registers `T` (usable from any thread) if it is not registered yet, then moves `value` in. Stamps
    /// the world's change tick (only as the changed tick when replacing).
    /// @code
    /// world.getResources().insert(Time{.delta = 0.F, .fixedDelta = 1.F / 60.F});
    /// @endcode
    /// @throws exceptions::ResourceRegistrationException When `T` is registered as main-thread only, or its name is
    /// already taken by another layout (a run-time resource with the same name).
    template <Resource T>
    void insert(T value);

    /// @brief Sets a main-thread-only C++ resource (the platform, the renderer), replacing its value if any.
    /// @details Like insert(), but registers `T` as main-thread only: systems using it must run on the main thread.
    /// @throws exceptions::ResourceRegistrationException When `T` is registered as usable from any thread, or its
    /// name is already taken by another layout.
    template <Resource T>
    void insertNonSend(T value);

    /// @return Whether the C++ resource holds a value; false if `T` is not registered in this world.
    template <Resource T>
    [[nodiscard]] bool has() const noexcept;

    /// @return The resource's value, or nullptr if it has none or `T` is not registered. Invalidated by the next
    /// insertion or removal of the resource. Empty types have no value: test them with has().
    template <Resource T>
    [[nodiscard]] const T* get() const noexcept;

    /// @return Write access to the resource's value, which marks it changed only when written through (`->`, `*`);
    /// an empty Mut if it has no value or `T` is not registered. Invalidated by the next insertion or removal of the
    /// resource. Empty types have no value: test them with has().
    template <Resource T>
    [[nodiscard]] Mut<T> getMut() noexcept;

    /// @return The resource's value. Invalidated by the next insertion or removal of the resource.
    /// @throws exceptions::MissingResourceException When it has no value or `T` is not registered.
    template <Resource T>
    [[nodiscard]] const T& require() const;

    /// @return Write access to the resource's value, which marks it changed only when written through (`->`, `*`).
    /// Invalidated by the next insertion or removal of the resource.
    /// @throws exceptions::MissingResourceException When it has no value or `T` is not registered.
    template <Resource T>
    [[nodiscard]] Mut<T> requireMut();

    /// @brief Destroys the C++ resource's value. The resource stays registered.
    /// @return False, doing nothing, if it has no value or `T` is not registered.
    template <Resource T>
    bool remove() noexcept;

    /// @return When the resource's value was inserted and last written, or nothing if it has no value or `T` is not
    /// registered.
    template <Resource T>
    [[nodiscard]] std::optional<ComponentTicks> getTicks() const noexcept;

    /// @brief Clamps the added and changed ticks of every resource to at most kMaxChangeAge older than `now`.
    /// Called by World::checkChangeTicks().
    void clampTicks(Tick now) noexcept;

  private:
    /// @return The world's change tick, stamped on every value inserted or written.
    [[nodiscard]] Tick getChangeTick() const noexcept;

    /// @brief Checks that `name` is not taken by another kind of type (WorldState::checkNameFree()). A member, so the
    /// templates, which only see a declaration of WorldState, can call it.
    /// @return Nothing when the name is free, or an ErrorKind::kNameCollision error.
    [[nodiscard]] Result<void> checkNameFree(std::string_view name) const;

    /// @brief Moves `value` into the slot of `T`, registering `T` with `options` if needed.
    template <Resource T>
    void insertWith(T value, ResourceOptions options);

    /// @brief Creates the slots of every registered resource up to `id` that has none yet.
    void createStorages(ResourceId id);

    /// @return The slot of the resource, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::ResourceStorage* findStorage(ResourceId id) noexcept;
    [[nodiscard]] const storage::ResourceStorage* findStorage(ResourceId id) const noexcept;

    const detail::WorldState* _state;  ///< The world's state, read for its change tick and components.
    ResourceRegistry _registry;        ///< Resources of this world, by id and name.
    /// @brief One slot per resource, indexed by ResourceId; a deque keeps their addresses, so a Mut stays valid
    /// when other resources are registered.
    std::deque<storage::ResourceStorage> _storages;
};
}  // namespace rtype::ecs

#include "Resources.tpp"

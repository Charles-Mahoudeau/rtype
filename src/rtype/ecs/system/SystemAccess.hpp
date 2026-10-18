/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemAccess
*/

#pragma once

#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief How a system uses one component, resource or event type.
struct TypeAccess {
    TypeId id{0};       ///< The type, by its id in the world.
    bool write{false};  ///< True when the system writes it, false when it only reads it.

    [[nodiscard]] constexpr bool operator==(const TypeAccess& other) const noexcept = default;
};

/// @brief Everything a system reads and writes, deduced from its parameters: what the scheduler orders systems by
/// (two systems conflict when one writes what the other reads or writes) and, later, runs disjoint ones in parallel.
/// @details Each type appears once per kind; adding it again as written turns a read into a write. Commands are not
/// listed: they are structural changes applied at the flush, after the system ran.
class RTYPE_ECS_API SystemAccess {
  public:
    SystemAccess() = default;
    ~SystemAccess() = default;
    SystemAccess(const SystemAccess& other) = default;
    SystemAccess& operator=(const SystemAccess& other) = default;
    SystemAccess(SystemAccess&& other) noexcept = default;
    SystemAccess& operator=(SystemAccess&& other) noexcept = default;

    /// @return The components the system reads or writes, in the order they were first added.
    [[nodiscard]] std::span<const TypeAccess> getComponents() const noexcept;

    /// @return The resources the system reads or writes, in the order they were first added.
    [[nodiscard]] std::span<const TypeAccess> getResources() const noexcept;

    /// @return The event types the system reads or sends, in the order they were first added.
    [[nodiscard]] std::span<const TypeAccess> getEvents() const noexcept;

    /// @return Whether the system uses a main-thread-only resource (NonSend, NonSendMut): it must run on the main
    /// thread.
    [[nodiscard]] bool isMainThread() const noexcept;

    /// @brief Records reading (`write` false) or writing a component.
    void addComponent(TypeId id, bool write);

    /// @brief Records reading or writing a resource.
    void addResource(TypeId id, bool write);

    /// @brief Records reading or sending an event type.
    void addEvent(TypeId id, bool write);

    /// @brief Records that the system must run on the main thread.
    void requireMainThread() noexcept;

  private:
    /// @brief Adds the type to `accesses`, or upgrades its entry to a write.
    static void add(std::vector<TypeAccess>& accesses, TypeId id, bool write);

    std::vector<TypeAccess> _components;  ///< Components read or written, one entry each.
    std::vector<TypeAccess> _resources;   ///< Resources read or written, one entry each.
    std::vector<TypeAccess> _events;      ///< Event types read or sent, one entry each.
    bool _mainThread{false};              ///< Whether the system must run on the main thread.
};
}  // namespace rtype::ecs

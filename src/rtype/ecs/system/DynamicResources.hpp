/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicResources
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief One resource a system uses, by id, and whether it writes it.
struct ResourceUse {
    ResourceId id{kInvalidResourceId};  ///< The resource, registered before the system is added.
    bool write{false};                  ///< True when the system may write it, false when it only reads it.
};

/// @brief The configuration of a DynamicResources parameter: the resources the system uses, given when it is added.
/// @details They are numbered in this order (DynamicResources::get(0) is the first).
struct ResourceList {
    std::vector<ResourceUse> resources;  ///< The resources, in order.
};

/// @brief A system parameter giving the bytes of resources chosen when the system is added, by id: for resources the
/// system does not know at compile time (those a script names).
///
/// @details Configured with a ResourceList (see SystemParam): the system's access set gets every resource of the list
/// as read or written, so the scheduler orders it as for `Res` and `ResMut`. A resource declared read refuses write
/// access (getMut() gives nothing), so the system writes nothing it did not declare. Main-thread resources are
/// refused when the system is added: a dynamic system may run on any thread.
/// @code
/// void run(rtype::ecs::DynamicResources resources) {
///     std::span<const std::byte> time = resources.get(0);
/// }
/// app.addSystem(rtype::ecs::Phase::kUpdate, "game.read", &run, rtype::ecs::ResourceList{{{timeId, false}}});
/// @endcode
class RTYPE_ECS_API DynamicResources {
  public:
    /// @param resources The world's resources; they must outlive the view.
    /// @param uses The resources of the system, in order; they must outlive the view.
    DynamicResources(Resources& resources, std::span<const ResourceUse> uses) noexcept;

    ~DynamicResources() = default;
    DynamicResources(const DynamicResources& other) noexcept = default;
    DynamicResources& operator=(const DynamicResources& other) noexcept = default;
    DynamicResources(DynamicResources&& other) noexcept = default;
    DynamicResources& operator=(DynamicResources&& other) noexcept = default;

    /// @return The number of resources, as given in the ResourceList.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The use at `index`; `index` must be below getCount().
    [[nodiscard]] const ResourceUse& getUse(std::size_t index) const noexcept;

    /// @return Whether the resource at `index` holds a value.
    [[nodiscard]] bool has(std::size_t index) const noexcept;

    /// @return The bytes of the resource at `index`; empty if it has no value or is an empty type (use has()).
    [[nodiscard]] std::span<const std::byte> get(std::size_t index) const noexcept;

    /// @brief Write access to the resource at `index`, marked changed now.
    /// @return The bytes of the value; empty if the resource was declared read, has no value or is an empty type.
    [[nodiscard]] std::span<std::byte> getMut(std::size_t index) noexcept;

  private:
    Resources* _resources;               ///< The world's resources.
    std::span<const ResourceUse> _uses;  ///< The system's resources.
};

/// @brief `DynamicResources`: configured with a ResourceList; the system keeps it.
template <>
struct RTYPE_ECS_API SystemParam<DynamicResources> {
    using Config = ResourceList;  ///< The resources to use, given when the system is added.
    using State = ResourceList;   ///< The same list, checked against the world.

    /// @return `config`, checked against the world's resources.
    /// @throws exceptions::ResourceRegistrationException When an id is not a registered resource, or the resource is
    /// main-thread only.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the resources for this run.
    [[nodiscard]] static DynamicResources get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds reading or writing every resource of the list.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace rtype::ecs

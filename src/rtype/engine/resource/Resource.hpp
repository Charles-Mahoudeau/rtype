/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Resource
*/

#pragma once

#include "engine/resource/Handle.hpp"

namespace rtype::engine::resource {

/// @brief Owns one backend resource and destroys it when going out of scope (RAII around a Handle).
///
/// @details Move-only: there is exactly one owner per resource. Code that only needs to refer to the resource
/// (an ECS component, a sprite) keeps its Handle, from getId(), instead.
///
/// @tparam Tag Kind of resource, see Handle.
/// @tparam Owner Backend that created the resource. It must provide `void destroy(Handle<Tag>)`.
///
/// @warning The owner must outlive every Resource it created.
template <typename Tag, typename Owner>
class Resource {
  public:
    /// @brief Takes ownership of a resource the owner just created.
    Resource(Owner& owner, Handle<Tag> handle) noexcept;
    /// @brief An empty Resource, owning nothing.
    Resource() = default;
    /// @brief Calls owner.destroy() if a resource is still owned.
    ~Resource();
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource(Resource&& other) noexcept;
    Resource& operator=(Resource&& other) noexcept;

    /// @return The handle of the owned resource, to refer to it without owning it.
    [[nodiscard]] Handle<Tag> getId() const noexcept;

    /// @return True while a resource is owned.
    [[nodiscard]] bool isValid() const noexcept;

    /// @brief Destroys the owned resource now, leaving this Resource empty.
    void reset() noexcept;

  private:
    Owner* _owner = nullptr;  ///< Backend to call destroy() on; null when empty.
    Handle<Tag> _handle{};    ///< The owned resource.
};
}  // namespace rtype::engine::resource

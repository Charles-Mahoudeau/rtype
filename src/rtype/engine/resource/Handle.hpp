/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Handle
*/

#pragma once

#include <cstdint>

namespace rtype::engine::resource {

/// @brief Identifies a resource owned by a backend (a texture in the renderer, a sound in the audio...).
///
/// @details A plain value: cheap to copy, safe to store in ECS components or to send over the network. It owns
/// nothing; the owning side is Resource. The backend keeps a table indexed by `index`, and bumps the slot's
/// generation each time it frees it, so a handle used after its resource was destroyed is detected instead of
/// silently reaching the slot's new resource.
///
/// @tparam Tag Empty type that makes each kind of handle a distinct type (a TextureId is not a BufferId).
template <typename Tag>
struct Handle {
    std::uint32_t index = 0;       ///< Slot in the backend's table.
    std::uint32_t generation = 0;  ///< Version of the slot; 0 means "no resource".

    [[nodiscard]] bool operator==(const Handle&) const = default;
};
}  // namespace rtype::engine::resource

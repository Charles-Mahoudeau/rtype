/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetId
*/

#include "plugins/backend/graphics/AssetId.hpp"

#include <cstdint>
#include <filesystem>

#include "rtype/ecs/type/NameHash.hpp"

namespace rtype::plugins::backend {

AssetId AssetId::fromPath(const std::filesystem::path& path) {
    const std::uint64_t hash = ecs::hashName(path.lexically_normal().generic_string());
    return AssetId{.value = hash == 0 ? 1 : hash};  // 0 means "no asset"
}

}  // namespace rtype::plugins::backend

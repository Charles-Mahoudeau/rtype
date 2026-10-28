/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetId
*/

#include "plugins/asset/AssetId.hpp"

#include <cstdint>
#include <filesystem>

#include "plugins/asset/AssetFiles.hpp"
#include "rtype/ecs/type/NameHash.hpp"

namespace rtype::plugins::asset {

AssetId AssetId::fromPath(const std::filesystem::path& path) {
    const std::uint64_t hash = ecs::hashName(normalizePath(path));
    return AssetId{.value = hash == 0 ? 1 : hash};
}

}  // namespace rtype::plugins::asset

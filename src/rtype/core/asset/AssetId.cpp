/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetId
*/

#include "core/asset/AssetId.hpp"

#include <cstdint>
#include <filesystem>

#include "core/hash/NameHash.hpp"

namespace rtype::core::asset {

AssetId AssetId::fromPath(const std::filesystem::path& path) {
    const std::uint64_t hash = hash::hashName(path.lexically_normal().generic_string());
    return AssetId{.value = hash == 0 ? 1 : hash};
}

}  // namespace rtype::core::asset

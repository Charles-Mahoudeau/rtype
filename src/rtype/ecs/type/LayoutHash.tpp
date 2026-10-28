/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LayoutHash
*/

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "core/hash/NameHash.hpp"
#include "rtype/ecs/type/TypeHooks.hpp"

namespace rtype::ecs {
template <typename Fields>
constexpr std::uint64_t hashLayout(std::string_view name, std::size_t size, std::size_t alignment,
                                   const TypeHooks& hooks, bool copyable, const Fields& fields) noexcept {
    std::uint64_t hash = core::hash::hashString(core::hash::kNameHashOffsetBasis, name);
    hash = core::hash::hashInteger(hash, size);
    hash = core::hash::hashInteger(hash, alignment);
    // One bit per hook that exists.
    hash =
        core::hash::hashInteger(hash, (hooks.construct != nullptr ? 1U : 0U) | (hooks.destruct != nullptr ? 2U : 0U) |
                                          (hooks.move != nullptr ? 4U : 0U) | (hooks.copy != nullptr ? 8U : 0U));
    hash = core::hash::hashInteger(hash, copyable ? 1U : 0U);
    hash = core::hash::hashInteger(hash, fields.size());
    for (const auto& field : fields) {
        // The length first, so that fields "ab" + "c" never hash like "a" + "bc".
        hash = core::hash::hashInteger(hash, field.name.size());
        hash = core::hash::hashString(hash, field.name);
        hash = core::hash::hashInteger(hash, static_cast<std::uint64_t>(field.type));
        hash = core::hash::hashInteger(hash, field.offset);
        hash = core::hash::hashInteger(hash, field.count);
    }
    return hash;
}
}  // namespace rtype::ecs

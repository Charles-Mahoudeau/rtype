/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceRegistry
*/

#include "ResourceRegistry.hpp"

#include <string_view>
#include <tl/expected.hpp>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/type/TypeRegistry.hpp"

namespace rtype::ecs {
namespace {
[[nodiscard]] std::string_view toThread(bool nonSend) noexcept { return nonSend ? "main" : "any"; }
}  // namespace

Result<void> checkSameThread(const ResourceInfo& registered, const ResourceInfo& requested) {
    if (registered.nonSend != requested.nonSend) {
        return tl::unexpected<Error>{detail::optionsMismatch(KindTraits<ResourceInfo>::kLabel, requested.type.name,
                                                             "thread", toThread(registered.nonSend),
                                                             toThread(requested.nonSend))};
    }
    return {};
}
}  // namespace rtype::ecs

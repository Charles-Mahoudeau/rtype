/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistry
*/

#include "ComponentRegistry.hpp"

#include <string_view>
#include <tl/expected.hpp>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeRegistry.hpp"

namespace rtype::ecs {
namespace {
[[nodiscard]] std::string_view toString(StorageKind storage) noexcept {
    switch (storage) {
        case StorageKind::kSparseSet:
            return "sparseSet";
        case StorageKind::kTable:
            return "table";
    }
    return "unknown";
}
}  // namespace

Result<void> checkSameStorage(const ComponentInfo& registered, const ComponentInfo& requested) {
    if (registered.storage != requested.storage) {
        return tl::unexpected<Error>{detail::optionsMismatch(KindTraits<ComponentInfo>::kLabel, requested.type.name,
                                                             "storage", toString(registered.storage),
                                                             toString(requested.storage))};
    }
    return {};
}
}  // namespace rtype::ecs

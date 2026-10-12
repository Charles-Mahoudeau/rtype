/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentInfo
*/

#include "ComponentInfo.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>
#include <tl/expected.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"

namespace rtype::ecs {
namespace {

/// @brief The bytes a field occupies inside its component.
struct FieldSpan {
    std::uint64_t begin;    ///< First byte.
    std::uint64_t end;      ///< One past the last byte.
    std::string_view name;  ///< Field name, for messages.
};

/// @return An ErrorKind::kInvalidComponent error about the named component.
[[nodiscard]] tl::unexpected<Error> invalid(std::string_view component, std::string_view problem) {
    return fail(ErrorKind::kInvalidComponent, std::format("Component '{}': {}", component, problem));
}

/// @brief Checks every field on its own, and collects the bytes each one occupies.
[[nodiscard]] Result<std::vector<FieldSpan>> checkFields(const ComponentInfo& info) {
    std::vector<FieldSpan> spans;
    spans.reserve(info.fields.size());
    std::unordered_set<std::string_view> names;
    for (const FieldInfo& field : info.fields) {
        if (field.name.empty()) {
            return invalid(info.name, std::format("the field at offset {} has no name", field.offset));
        }
        if (!names.insert(field.name).second) {
            return invalid(info.name, std::format("field '{}' is declared twice", field.name));
        }
        if (field.count == 0) {
            return invalid(info.name, std::format("field '{}' has a count of 0", field.name));
        }
        const FieldLayout layout = getFieldLayout(field.type);
        if (field.offset % layout.alignment != 0) {
            return invalid(info.name, std::format("field '{}' at offset {} is not aligned to {} bytes", field.name,
                                                  field.offset, layout.alignment));
        }
        const std::uint64_t end = std::uint64_t{field.offset} + (std::uint64_t{layout.size} * field.count);
        if (end > info.size) {
            return invalid(info.name, std::format("field '{}' ends at byte {}, past the component size {}", field.name,
                                                  end, info.size));
        }
        spans.push_back(FieldSpan{.begin = field.offset, .end = end, .name = field.name});
    }
    return spans;
}

/// @brief Checks that no two fields share a byte.
[[nodiscard]] Result<void> checkOverlaps(std::string_view component, std::vector<FieldSpan> spans) {
    std::ranges::sort(spans, {}, &FieldSpan::begin);
    for (std::size_t i = 1; i < spans.size(); ++i) {
        const FieldSpan& previous = spans.at(i - 1);
        const FieldSpan& current = spans.at(i);
        if (previous.end > current.begin) {
            return invalid(component, std::format("fields '{}' and '{}' overlap", previous.name, current.name));
        }
    }
    return {};
}

}  // namespace

Result<void> validate(const ComponentInfo& info) {
    if (info.name.empty()) {
        return fail(ErrorKind::kInvalidComponent, "A component needs a name");
    }
    if (!std::has_single_bit(info.alignment)) {
        return invalid(info.name, std::format("alignment {} is not a power of two", info.alignment));
    }
    if (info.size % info.alignment != 0) {
        return invalid(info.name,
                       std::format("size {} is not a multiple of its alignment {}", info.size, info.alignment));
    }
    if (info.size == 0 && !info.fields.empty()) {
        return invalid(info.name, "a tag (size 0) cannot declare fields");
    }
    if (info.presence == Presence::kClientOnly && info.replication != ReplicationMode::kNone) {
        return invalid(info.name, "a client-only component cannot be replicated");
    }
    if (info.replication != ReplicationMode::kNone && info.size != 0 && info.fields.empty()) {
        return invalid(info.name, "a replicated component needs fields to serialize");
    }
    Result<std::vector<FieldSpan>> spans = checkFields(info);
    if (!spans) {
        return tl::unexpected<Error>{std::move(spans.error())};
    }
    return checkOverlaps(info.name, std::move(*spans));
}
}  // namespace rtype::ecs

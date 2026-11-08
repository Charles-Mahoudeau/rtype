/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldAdmin
*/

#include "rtype/replication/admin/WorldAdmin.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <format>
#include <functional>
#include <istream>
#include <locale>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tl/expected.hpp>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/entity/Name.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"

namespace rtype::replication::admin {
namespace net = network::admin;

namespace {
constexpr std::string_view kNameComponent = "rtype.Name";
constexpr std::size_t kMaxListedValues = 32;  ///< Values of an array shown in one field's text.
constexpr std::size_t kMaxComponentsPerReply = 255;

net::ErrorReply error(const net::ErrorCode code) { return net::ErrorReply{.code = code}; }

ecs::Entity entityOf(const std::uint64_t bits) {
    return ecs::Entity{static_cast<std::uint32_t>(bits), static_cast<std::uint32_t>(bits >> 32U)};
}

/// @return The value of type @p T at @p offset in @p bytes; zero when it does not lie in them.
template <typename T>
T load(const std::span<const std::byte> bytes, const std::size_t offset = 0) {
    T value{};
    if (offset <= bytes.size() && sizeof(T) <= bytes.size() - offset) {
        std::memcpy(&value, bytes.subspan(offset, sizeof(T)).data(), sizeof(T));
    }
    return value;
}

template <typename T>
void append(std::vector<std::byte>& bytes, const T& value) {
    const std::span<const std::byte> raw = std::as_bytes(std::span{&value, 1});
    bytes.insert(bytes.end(), raw.begin(), raw.end());
}

std::string_view typeName(const ecs::FieldType type) noexcept {
    switch (type) {
        case ecs::FieldType::kBool:
            return "bool";
        case ecs::FieldType::kI8:
            return "i8";
        case ecs::FieldType::kI16:
            return "i16";
        case ecs::FieldType::kI32:
            return "i32";
        case ecs::FieldType::kI64:
            return "i64";
        case ecs::FieldType::kU8:
            return "u8";
        case ecs::FieldType::kU16:
            return "u16";
        case ecs::FieldType::kU32:
            return "u32";
        case ecs::FieldType::kU64:
            return "u64";
        case ecs::FieldType::kF32:
            return "f32";
        case ecs::FieldType::kF64:
            return "f64";
        case ecs::FieldType::kVec2:
            return "vec2";
        case ecs::FieldType::kVec3:
            return "vec3";
        case ecs::FieldType::kVec4:
            return "vec4";
        case ecs::FieldType::kEntity:
            return "entity";
        case ecs::FieldType::kAssetId:
            return "asset";
        case ecs::FieldType::kText:
            return "text";
        case ecs::FieldType::kString:
            return "string";
    }
    return "?";
}

/// @return The text of the value of type @p type in the bytes @p where.
std::string valueText(const ecs::FieldType type, const std::span<const std::byte> where) {
    switch (type) {
        case ecs::FieldType::kBool:
            return load<std::uint8_t>(where) != 0 ? "true" : "false";
        case ecs::FieldType::kI8:
            return std::to_string(static_cast<int>(load<std::int8_t>(where)));
        case ecs::FieldType::kI16:
            return std::to_string(load<std::int16_t>(where));
        case ecs::FieldType::kI32:
            return std::to_string(load<std::int32_t>(where));
        case ecs::FieldType::kI64:
            return std::to_string(load<std::int64_t>(where));
        case ecs::FieldType::kU8:
            return std::to_string(static_cast<unsigned>(load<std::uint8_t>(where)));
        case ecs::FieldType::kU16:
            return std::to_string(load<std::uint16_t>(where));
        case ecs::FieldType::kU32:
            return std::to_string(load<std::uint32_t>(where));
        case ecs::FieldType::kEntity:
        case ecs::FieldType::kAssetId:
        case ecs::FieldType::kU64:
            return std::to_string(load<std::uint64_t>(where));
        case ecs::FieldType::kF32:
            return std::format("{}", load<float>(where));
        case ecs::FieldType::kF64:
            return std::format("{}", load<double>(where));
        case ecs::FieldType::kVec2:
            return std::format("{} {}", load<float>(where), load<float>(where, 4));
        case ecs::FieldType::kVec3:
            return std::format("{} {} {}", load<float>(where), load<float>(where, 4), load<float>(where, 8));
        case ecs::FieldType::kVec4:
            return std::format("{} {} {} {}", load<float>(where), load<float>(where, 4), load<float>(where, 8),
                               load<float>(where, 12));
        case ecs::FieldType::kText:
            return std::string{static_cast<char>(load<std::uint8_t>(where))};
        case ecs::FieldType::kString:
            return std::string{load<ecs::String>(where).view()};
    }
    return {};
}

bool inBounds(const std::span<const std::byte> value, const std::size_t offset, const std::size_t size) noexcept {
    return offset <= value.size() && size <= value.size() - offset;
}

/// @return What a tool shows of one field of the component value @p value; nothing when the field does not lie in it.
std::optional<net::FieldView> describeField(const ecs::FieldInfo& field, const std::span<const std::byte> value) {
    const ecs::FieldLayout layout = ecs::getFieldLayout(field.type);
    net::FieldView view;
    view.name = field.name;
    if (field.list != nullptr) {
        if (!inBounds(value, field.offset, field.list->size)) {
            return std::nullopt;
        }
        const std::span<const std::byte> items = field.list->elements(value.subspan(field.offset).data());
        view.type = std::format("list<{}>", typeName(field.type));
        view.count = 1;
        if (field.type == ecs::FieldType::kText) {
            for (const std::byte item : items) {
                view.value.push_back(static_cast<char>(item));
            }
            view.editable = true;
        } else {
            view.value = std::format("<{} items>", layout.size == 0 ? 0 : items.size() / layout.size);
        }
        return view;
    }
    if (layout.size == 0 || !inBounds(value, field.offset, static_cast<std::size_t>(layout.size) * field.count)) {
        return std::nullopt;
    }
    view.type = std::string{typeName(field.type)};
    view.count = static_cast<std::uint16_t>(std::min<std::uint32_t>(field.count, 0xFFFF));
    view.editable = field.type != ecs::FieldType::kEntity && field.type != ecs::FieldType::kText;
    if (field.count <= 1) {
        view.value = valueText(field.type, value.subspan(field.offset, layout.size));
        return view;
    }
    view.value = "[";
    for (std::uint32_t i = 0; i < field.count && i < kMaxListedValues; ++i) {
        const std::size_t at = field.offset + (static_cast<std::size_t>(i) * layout.size);
        view.value += (i == 0 ? "" : ", ") + valueText(field.type, value.subspan(at, layout.size));
    }
    view.value += field.count > kMaxListedValues ? ", ...]" : "]";
    return view;
}

std::string trimmed(std::string_view text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    return std::string{text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1)};
}

/// @return @p text as a number of type @p T; nothing unless all of it reads and the number fits.
template <typename T>
std::optional<T> parseInteger(const std::string_view text) {
    std::conditional_t<std::is_signed_v<T>, long long, unsigned long long> value = 0;
    // The standard API takes a range of pointers.
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-suspicious-stringview-data-usage)
    const char* const last = text.data() + text.size();
    const auto [end, code] = std::from_chars(text.data(), last, value);
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-suspicious-stringview-data-usage)
    if (code != std::errc{} || end != last || !std::in_range<T>(value)) {
        return std::nullopt;
    }
    return static_cast<T>(value);
}

std::optional<float> parseFloat(const std::string& text) {
    std::istringstream in{text};
    in.imbue(std::locale::classic());
    double value = 0;
    in >> value;
    if (in.fail()) {
        return std::nullopt;
    }
    in >> std::ws;
    const auto narrowed = static_cast<float>(value);
    if (!in.eof() || !std::isfinite(value) || !std::isfinite(narrowed)) {
        return std::nullopt;
    }
    return narrowed;
}

std::optional<double> parseDouble(const std::string& text) {
    std::istringstream in{text};
    in.imbue(std::locale::classic());
    double value = 0;
    in >> value;
    if (in.fail()) {
        return std::nullopt;
    }
    in >> std::ws;
    if (!in.eof() || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

/// @return The bytes of @p count floats read from the numbers in @p text, separated by spaces or commas.
std::optional<std::vector<std::byte>> parseFloats(const std::string_view text, const std::size_t count) {
    std::string spaced{text};
    std::ranges::replace(spaced, ',', ' ');
    std::istringstream in{spaced};
    std::vector<std::byte> bytes;
    std::string token;
    std::size_t found = 0;
    while (in >> token) {
        const auto number = parseFloat(token);
        if (!number || ++found > count) {
            return std::nullopt;
        }
        append(bytes, *number);
    }
    if (found != count) {
        return std::nullopt;
    }
    return bytes;
}

template <typename T>
std::optional<std::vector<std::byte>> bytesOf(const std::optional<T>& value) {
    if (!value) {
        return std::nullopt;
    }
    std::vector<std::byte> bytes;
    append(bytes, *value);
    return bytes;
}

/// @return The bytes of one value of @p type read from @p raw; nothing when it does not read as that type.
std::optional<std::vector<std::byte>> parseValue(const ecs::FieldType type, const std::string_view raw) {
    const std::string text = trimmed(raw);
    switch (type) {
        case ecs::FieldType::kBool: {
            if (text != "true" && text != "false" && text != "1" && text != "0") {
                return std::nullopt;
            }
            return bytesOf(std::optional<std::uint8_t>{text == "true" || text == "1" ? 1 : 0});
        }
        case ecs::FieldType::kI8:
            return bytesOf(parseInteger<std::int8_t>(text));
        case ecs::FieldType::kI16:
            return bytesOf(parseInteger<std::int16_t>(text));
        case ecs::FieldType::kI32:
            return bytesOf(parseInteger<std::int32_t>(text));
        case ecs::FieldType::kI64:
            return bytesOf(parseInteger<std::int64_t>(text));
        case ecs::FieldType::kU8:
            return bytesOf(parseInteger<std::uint8_t>(text));
        case ecs::FieldType::kU16:
            return bytesOf(parseInteger<std::uint16_t>(text));
        case ecs::FieldType::kU32:
            return bytesOf(parseInteger<std::uint32_t>(text));
        case ecs::FieldType::kAssetId:
        case ecs::FieldType::kU64:
            return bytesOf(parseInteger<std::uint64_t>(text));
        case ecs::FieldType::kF32:
            return bytesOf(parseFloat(text));
        case ecs::FieldType::kF64:
            return bytesOf(parseDouble(text));
        case ecs::FieldType::kVec2:
            return parseFloats(text, 2);
        case ecs::FieldType::kVec3:
            return parseFloats(text, 3);
        case ecs::FieldType::kVec4:
            return parseFloats(text, 4);
        case ecs::FieldType::kString:
            return bytesOf(std::optional<ecs::String>{ecs::String{std::string_view{raw}}});
        case ecs::FieldType::kEntity:
        case ecs::FieldType::kText:
            return std::nullopt;
    }
    return std::nullopt;
}

/// @return The entities that have a component, by slot index.
std::vector<ecs::Entity> listEntities(const ecs::World& world) {
    const ecs::Components& components = world.getComponents();
    std::vector<ecs::Entity> entities;
    const std::size_t count = components.getRegistry().getCount();
    for (std::size_t index = 0; index < count; ++index) {
        const auto* pool = components.findPool(static_cast<ecs::ComponentId>(index));
        if (pool == nullptr || components.getRegistry().findById(static_cast<ecs::ComponentId>(index)) == nullptr) {
            continue;
        }
        const auto members = pool->getEntities();
        entities.insert(entities.end(), members.begin(), members.end());
    }
    std::ranges::sort(entities, {}, &ecs::Entity::getBits);
    std::ranges::sort(entities, {}, &ecs::Entity::getIndex);
    const auto duplicates = std::ranges::unique(entities);
    entities.erase(duplicates.begin(), duplicates.end());
    return entities;
}

/// @return The components @p entity has, by registry order.
std::vector<ecs::ComponentId> componentsOf(const ecs::World& world, const ecs::Entity entity) {
    const ecs::Components& components = world.getComponents();
    std::vector<ecs::ComponentId> ids;
    const std::size_t count = components.getRegistry().getCount();
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        if (components.getRegistry().findById(id) != nullptr && components.has(entity, id)) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::string nameOf(const ecs::World& world, const ecs::Entity entity) {
    const ecs::Components& components = world.getComponents();
    const std::optional<ecs::ComponentId> id = components.find(kNameComponent);
    if (!id) {
        return {};
    }
    const std::span<const std::byte> value = components.get(entity, *id);
    if (value.size() < sizeof(ecs::Name)) {
        return {};
    }
    return std::string{load<ecs::Name>(value).value.view()};
}
}  // namespace

WorldAdmin::WorldAdmin(const ecs::World& world, ecs::Schedules* schedules) noexcept
    : _world{&world}, _schedules{schedules} {}

void WorldAdmin::bind(ecs::Commands* commands) noexcept { _commands = commands; }

std::optional<net::Reply> WorldAdmin::handle(const net::ExtensionRequest& request) {
    const std::optional<net::WorldRequest> decoded = net::decodeWorldRequest(request);
    if (!decoded) {
        const bool known = request.type >= static_cast<std::uint8_t>(net::WorldMessage::kEntities) &&
                           request.type <= static_cast<std::uint8_t>(net::WorldMessage::kRun);
        if (!known) {
            return std::nullopt;
        }
        return error(net::ErrorCode::kMalformed);
    }
    net::Reply reply = std::visit(
        [this](const auto& value) -> net::Reply {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, net::EntitiesRequest>) {
                return entities(value);
            } else if constexpr (std::is_same_v<T, net::InspectRequest>) {
                return inspect(value);
            } else if constexpr (std::is_same_v<T, net::SetFieldRequest>) {
                return setField(value);
            } else if constexpr (std::is_same_v<T, net::SpawnRequest>) {
                return spawn(value);
            } else if constexpr (std::is_same_v<T, net::DespawnRequest>) {
                return despawn(value);
            } else {
                return run(value);
            }
        },
        *decoded);
    return reply;
}

net::Reply WorldAdmin::entities(const net::EntitiesRequest& request) const {
    const std::vector<ecs::Entity> all = listEntities(*_world);
    net::EntitiesReply reply;
    reply.total = static_cast<std::uint32_t>(all.size());
    reply.first = std::min<std::uint32_t>(request.first, reply.total);
    std::size_t used = net::entitiesPrefixSize();
    for (std::size_t i = reply.first; i < all.size(); ++i) {
        net::EntityEntry entry{
            .entity = all.at(i).getBits(),
            .name = nameOf(*_world, all.at(i)),
            .componentCount =
                static_cast<std::uint16_t>(std::min<std::size_t>(componentsOf(*_world, all.at(i)).size(), 0xFFFF)),
        };
        const std::size_t size = net::encodedSize(entry);
        if (used + size > net::kMaxWorldBody) {
            break;
        }
        used += size;
        reply.entries.push_back(std::move(entry));
    }
    return net::encodeWorldReply(reply);
}

net::Reply WorldAdmin::inspect(const net::InspectRequest& request) const {
    const ecs::Entity entity = entityOf(request.entity);
    if (!_world->isAlive(entity)) {
        return error(net::ErrorCode::kUnknownEntity);
    }
    const std::vector<ecs::ComponentId> ids = componentsOf(*_world, entity);
    net::InspectReply reply;
    reply.entity = request.entity;
    reply.name = nameOf(*_world, entity);
    reply.total = static_cast<std::uint16_t>(std::min<std::size_t>(ids.size(), 0xFFFF));
    reply.next = std::min<std::uint16_t>(request.firstComponent, reply.total);
    std::size_t used = net::inspectPrefixSize(reply.name.size());
    const ecs::Components& components = _world->getComponents();
    for (std::size_t i = reply.next; i < ids.size() && reply.components.size() < kMaxComponentsPerReply; ++i) {
        const ecs::ComponentInfo* info = components.getRegistry().findById(ids.at(i));
        if (info == nullptr) {
            continue;
        }
        const std::span<const std::byte> value = components.get(entity, ids.at(i));
        net::ComponentView view{.name = info->type.name, .fields = {}};
        for (const ecs::FieldInfo& field : info->type.fields) {
            if (std::optional<net::FieldView> described = describeField(field, value)) {
                view.fields.push_back(std::move(*described));
            }
        }
        std::size_t size = net::encodedSize(view);
        if (used + size > net::kMaxWorldBody && !reply.components.empty()) {
            break;  // the rest comes in the next reply
        }
        while (used + size > net::kMaxWorldBody && !view.fields.empty()) {
            view.fields.pop_back();  // a component too big alone loses its last fields
            size = net::encodedSize(view);
        }
        used += size;
        reply.components.push_back(std::move(view));
        reply.next = static_cast<std::uint16_t>(i + 1);
    }
    if (reply.components.empty()) {
        reply.next = reply.total;
    }
    return net::encodeWorldReply(reply);
}

namespace {
using Edit = std::function<void(std::span<std::byte>)>;

/// @return What changes a text list field into @p text, or why it cannot.
tl::expected<Edit, net::ErrorCode> makeListEdit(const ecs::FieldInfo& field, const net::SetFieldRequest& request,
                                                const std::span<const std::byte> current) {
    if (field.type != ecs::FieldType::kText || request.index != 0 ||
        !inBounds(current, field.offset, field.list->size)) {
        return tl::make_unexpected(net::ErrorCode::kBadValue);
    }
    std::vector<std::byte> text;
    for (const char c : request.value) {
        text.push_back(static_cast<std::byte>(c));
    }
    return Edit{[offset = field.offset, size = field.list->size, list = field.list,
                 text = std::move(text)](std::span<std::byte> value) {
        if (inBounds(value, offset, size)) {
            list->assign(value.subspan(offset).data(), text);
        }
    }};
}

/// @return What writes the value of @p request in a plain field, or why it cannot.
tl::expected<Edit, net::ErrorCode> makeValueEdit(const ecs::FieldInfo& field, const net::SetFieldRequest& request,
                                                 const std::span<const std::byte> current) {
    const ecs::FieldLayout layout = ecs::getFieldLayout(field.type);
    if (request.index >= std::max<std::uint32_t>(field.count, 1)) {
        return tl::make_unexpected(net::ErrorCode::kUnknownField);
    }
    const std::size_t offset = field.offset + (static_cast<std::size_t>(request.index) * layout.size);
    if (layout.size == 0 || !inBounds(current, offset, layout.size)) {
        return tl::make_unexpected(net::ErrorCode::kUnknownField);
    }
    std::optional<std::vector<std::byte>> bytes = parseValue(field.type, request.value);
    if (!bytes || bytes->size() != layout.size) {
        return tl::make_unexpected(net::ErrorCode::kBadValue);
    }
    return Edit{[offset, bytes = std::move(*bytes)](std::span<std::byte> value) {
        if (inBounds(value, offset, bytes.size())) {
            std::memcpy(value.subspan(offset, bytes.size()).data(), bytes.data(), bytes.size());
        }
    }};
}
}  // namespace

net::Reply WorldAdmin::setField(const net::SetFieldRequest& request) {
    if (_commands == nullptr) {
        return error(net::ErrorCode::kUnsupported);
    }
    const ecs::Entity entity = entityOf(request.entity);
    if (!_world->isAlive(entity)) {
        return error(net::ErrorCode::kUnknownEntity);
    }
    const ecs::Components& components = _world->getComponents();
    const std::optional<ecs::ComponentId> id = components.find(request.component);
    const ecs::ComponentInfo* info = id ? components.getRegistry().findById(*id) : nullptr;
    if (!id || info == nullptr || !components.has(entity, *id)) {
        return error(net::ErrorCode::kUnknownComponent);
    }
    const auto field = std::ranges::find(info->type.fields, request.field, &ecs::FieldInfo::name);
    if (field == info->type.fields.end()) {
        return error(net::ErrorCode::kUnknownField);
    }
    const std::span<const std::byte> current = components.get(entity, *id);
    tl::expected<Edit, net::ErrorCode> edit =
        field->list != nullptr ? makeListEdit(*field, request, current) : makeValueEdit(*field, request, current);
    if (!edit) {
        return error(edit.error());
    }
    if (!_commands->patch(entity, *id, std::move(*edit))) {
        return error(net::ErrorCode::kBadValue);
    }
    return net::encodeWorldReply(net::SetFieldReply{});
}

net::Reply WorldAdmin::spawn(const net::SpawnRequest& request) {
    if (_commands == nullptr) {
        return error(net::ErrorCode::kUnsupported);
    }
    try {
        ecs::Commands::EntityCommands spawned = _commands->spawn();
        spawned.add(ecs::Name{.value = ecs::String{request.name.empty() ? std::string_view{"Entity"} : request.name}});
        return net::encodeWorldReply(net::SpawnReply{.entity = spawned.getEntity().getBits()});
    } catch (const std::exception&) {
        return error(net::ErrorCode::kUnsupported);  // no entity left
    }
}

net::Reply WorldAdmin::despawn(const net::DespawnRequest& request) {
    if (_commands == nullptr) {
        return error(net::ErrorCode::kUnsupported);
    }
    const ecs::Entity entity = entityOf(request.entity);
    if (!_world->isAlive(entity)) {
        return error(net::ErrorCode::kUnknownEntity);
    }
    _commands->despawn(entity);
    return net::encodeWorldReply(net::DespawnReply{.entity = request.entity});
}

net::Reply WorldAdmin::run(const net::RunRequest& request) {
    if (_schedules == nullptr) {
        return error(net::ErrorCode::kUnsupported);
    }
    switch (request.action) {
        case net::RunAction::kQuery:
            break;
        case net::RunAction::kPause:
            _schedules->setPaused(true);
            break;
        case net::RunAction::kResume:
            _schedules->setPaused(false);
            break;
        case net::RunAction::kStep:
            _schedules->step();
            break;
    }
    return net::encodeWorldReply(net::RunReply{.paused = _schedules->isPaused()});
}

}  // namespace rtype::replication::admin

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldProtocol
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"

/// The world messages of the admin channel (types `0x20` to `0x25`): looking at the entities of a running server and
/// changing them. The messages are plain data, the network module knows nothing of the ECS: an entity is the 64 bits of
/// its handle, a value is a text. The server side that answers is in `replication/admin/`. The wire format is in
/// `docs/network/design.md` (Admin channel, world messages).
///
/// A request travels as an `ExtensionRequest` and its reply as an `ExtensionReply` (type + `kReplyFlag`): encode and
/// decode them here. All sizes are bounded so that a reply fits in one datagram.
namespace rtype::network::admin {

/// @brief The type byte of a world request.
enum class WorldMessage : std::uint8_t {
    kEntities = 0x20,  ///< A page of the entity list.
    kInspect = 0x21,   ///< The components of one entity, with their values as text.
    kSetField = 0x22,  ///< Sets one value of one component.
    kSpawn = 0x23,     ///< Spawns an entity.
    kDespawn = 0x24,   ///< Despawns an entity.
    kRun = 0x25,       ///< Pauses, resumes or steps the simulation, or tells its state.
};

/// @brief The most bytes of the body of a world reply.
inline constexpr std::size_t kMaxWorldBody = 1100;
/// @brief The longest name of an entity, a component or a field that a message carries; a longer one is cut.
inline constexpr std::size_t kMaxWorldName = 63;
/// @brief The longest text of one value; a longer one is cut and ends with "...".
inline constexpr std::size_t kMaxWorldValue = 96;

/// @brief The request for a page of the entity list: the entities that have a component, by index.
struct EntitiesRequest {
    std::uint32_t first{0};  ///< How many entities to skip.
};
/// @brief The request for the components of an entity.
struct InspectRequest {
    std::uint64_t entity{0};          ///< The handle of the entity.
    std::uint16_t firstComponent{0};  ///< How many components to skip, to read the rest of a long list.
};
/// @brief The request that sets one value.
struct SetFieldRequest {
    std::uint64_t entity{0};  ///< The handle of the entity.
    std::string component;    ///< The full name of the component ("rtype.Transform").
    std::string field;        ///< The name of the field.
    std::uint16_t index{0};   ///< Which value of an array field; 0 for any other.
    std::string value;        ///< The new value as text: "1.5", "true", "1 2 3" for a vector.
};
/// @brief The request that spawns an entity. It gets a Name, since an entity without a component cannot be listed.
struct SpawnRequest {
    std::string name;  ///< Its name; empty gives "Entity".
};
/// @brief The request that despawns an entity.
struct DespawnRequest {
    std::uint64_t entity{0};  ///< The handle of the entity.
};
/// @brief What a run request does.
enum class RunAction : std::uint8_t {
    kQuery = 0,   ///< Nothing: tells the state.
    kPause = 1,   ///< Freezes FixedUpdate and Update.
    kResume = 2,  ///< Lets them run again.
    kStep = 3,    ///< While paused, runs one frame.
};
/// @brief The request that controls the simulation.
struct RunRequest {
    RunAction action{RunAction::kQuery};  ///< What to do.
};

/// @brief What a tool may ask of the world.
using WorldRequest =
    std::variant<EntitiesRequest, InspectRequest, SetFieldRequest, SpawnRequest, DespawnRequest, RunRequest>;

/// @brief One entity of the list.
struct EntityEntry {
    std::uint64_t entity{0};          ///< The handle.
    std::string name;                 ///< Its Name component, empty when it has none.
    std::uint16_t componentCount{0};  ///< How many components it has.
};
/// @brief A page of the entity list. A page holds as many entries as fit (about 15): ask again from `first` + count.
struct EntitiesReply {
    std::uint32_t total{0};            ///< How many entities the world has (those with a component).
    std::uint32_t first{0};            ///< The index of the first entry.
    std::vector<EntityEntry> entries;  ///< As many as fit, by index.
};
/// @brief One field of a component, as the tool shows it.
struct FieldView {
    std::string name;        ///< The field's name.
    std::string type;        ///< "bool", "i32", "f32", "vec3", "entity", "string", "text", "list<f32>"...
    std::uint16_t count{1};  ///< How many values: 1 for a scalar, N for an array.
    std::string value;       ///< The value(s) as text; "[a, b]" for an array, "<n items>" for a list.
    bool editable{false};    ///< Whether a set request can change it.
};
/// @brief One component of an entity.
struct ComponentView {
    std::string name;               ///< The full name ("rtype.Transform").
    std::vector<FieldView> fields;  ///< Its fields, as many as fit.
};
/// @brief The components of an entity, as many as fit: ask again with `firstComponent` = `next` while `next` < `total`.
struct InspectReply {
    std::uint64_t entity{0};                ///< The handle.
    std::string name;                       ///< Its Name component, empty when it has none.
    std::uint16_t total{0};                 ///< How many components it has.
    std::uint16_t next{0};                  ///< The index after the last component of this reply.
    std::vector<ComponentView> components;  ///< From `firstComponent`.
};
/// @brief The reply to a set request that was accepted: the change is made before the next frame's Update.
struct SetFieldReply {};
/// @brief The reply to a spawn request.
struct SpawnReply {
    std::uint64_t entity{0};  ///< The handle of the new entity (alive from the next frame's PreUpdate on).
};
/// @brief The reply to a despawn request.
struct DespawnReply {
    std::uint64_t entity{0};  ///< The handle of the entity removed.
};
/// @brief The reply to a run request.
struct RunReply {
    bool paused{false};  ///< Whether the simulation is paused after the request.
};

/// @brief What the world answers.
using WorldReply = std::variant<EntitiesReply, InspectReply, SetFieldReply, SpawnReply, DespawnReply, RunReply>;

/// @return How many bytes @p entry takes in a reply.
[[nodiscard]] RTYPE_NETWORK_API std::size_t encodedSize(const EntityEntry& entry) noexcept;
/// @return How many bytes @p component takes in a reply.
[[nodiscard]] RTYPE_NETWORK_API std::size_t encodedSize(const ComponentView& component) noexcept;
/// @return How many bytes of an entities reply are not entries.
[[nodiscard]] RTYPE_NETWORK_API std::size_t entitiesPrefixSize() noexcept;
/// @return How many bytes of an inspect reply are not components, with an entity name of @p nameSize bytes.
[[nodiscard]] RTYPE_NETWORK_API std::size_t inspectPrefixSize(std::size_t nameSize) noexcept;

/// @brief Writes a world request as an extension request.
/// @return Its type and body. Names above kMaxWorldName and values above kMaxWorldValue are cut.
[[nodiscard]] RTYPE_NETWORK_API ExtensionRequest encodeWorldRequest(const WorldRequest& request);

/// @brief Reads a world request. Never throws, never asserts.
/// @return The request, or nothing when the type is not a world request or the body is invalid.
[[nodiscard]] RTYPE_NETWORK_API std::optional<WorldRequest> decodeWorldRequest(const ExtensionRequest& request);

/// @brief Writes a world reply as an extension reply.
/// @return Its type (the request's plus `kReplyFlag`) and body; an empty body of that type when the content is above
/// kMaxWorldBody.
[[nodiscard]] RTYPE_NETWORK_API ExtensionReply encodeWorldReply(const WorldReply& reply);

/// @brief Reads a world reply. Never throws, never asserts.
/// @return The reply, or nothing when the type is not a world reply or the body is invalid.
[[nodiscard]] RTYPE_NETWORK_API std::optional<WorldReply> decodeWorldReply(const ExtensionReply& reply);

}  // namespace rtype::network::admin

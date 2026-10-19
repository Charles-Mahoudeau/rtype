/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ManifestMessages
*/

#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "rtype/network/bytes/Codec.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/input/InputKind.hpp"

namespace rtype::replication::manifest {
/// @brief The longest name of an entry, as its length is a `u8`.
inline constexpr std::size_t kMaxNameLength = 255;

/// @brief Server to client: a replicated component.
struct ManifestComponent {
    static constexpr auto kKind = MessageType::kManifestComponent;  ///< The kind byte.
    std::uint16_t shortId{0};                                       ///< Position in the manifest, sorted by name.
    std::string name;                                               ///< The component's name.
    std::uint64_t layoutHash{0};                                    ///< The component's layout hash.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.shortId);
        stream.string(self.name, kMaxNameLength);
        stream.value(self.layoutHash);
    }
};

/// @brief Server to client: an input action.
struct ManifestInputAction {
    static constexpr auto kKind = MessageType::kManifestInputAction;  ///< The kind byte.
    std::uint8_t index{0};                                            ///< Position among the actions, by name.
    std::string name;                                                 ///< The action's name.
    input::InputKind kind{input::InputKind::kButton};                 ///< What the action carries.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.index);
        stream.string(self.name, kMaxNameLength);
        stream.value(self.kind);
    }
};

/// @brief Server to client: the last message of the manifest, with the settings the client needs.
struct ManifestEnd {
    static constexpr auto kKind = MessageType::kManifestEnd;  ///< The kind byte.
    std::uint16_t entryCount{0};                              ///< Components and actions sent before this message.
    std::uint64_t hash{0};                                    ///< The manifest hash.
    std::uint8_t tickRate{0};                                 ///< Simulation ticks per second.
    std::uint8_t snapshotRate{0};                             ///< Snapshots per second.
    std::uint16_t reconnectionGraceSeconds{0};                ///< How long a lost session waits for its client.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.entryCount);
        stream.value(self.hash);
        stream.value(self.tickRate);
        stream.value(self.snapshotRate);
        stream.value(self.reconnectionGraceSeconds);
    }
};

/// @brief Encodes and decodes the messages of the manifest.
using ManifestCodec =
    network::bytes::Codec<MessageType, std::variant<ManifestComponent, ManifestInputAction, ManifestEnd>>;
}  // namespace rtype::replication::manifest

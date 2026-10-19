/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Assets
*/

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rtype::plugins::backend {
/// @brief What Assets needs to know about one kind of asset: specialize it next to the asset's id.
///
/// @details A specialization provides:
/// - `Info`: what is known about a loaded asset (a texture's size...);
/// - `Owner`: the backend that creates and destroys the asset, with a `destroy(Id)` member (IRenderer, IAudio).
template <typename Id>
struct AssetTraits;

/// @brief The loaded assets of one kind (textures, later sounds): each one known by its name (its path, for a file)
/// and by the id its backend gave it, with what is known about it.
///
/// @details Inserted in the world as an ECS resource, so systems find an asset by name and components keep only its
/// id. The backend owns the asset itself: unload() asks it to destroy one, and the backend destroys every asset left
/// when it is destroyed. Lookups are linear, which suits the few hundred assets of a game.
/// @tparam Id The asset's id (graphics::TextureId): a plain value with `operator==`.
template <typename Id>
class Assets {
  public:
    /// @brief What is known about one asset.
    using Info = AssetTraits<Id>::Info;

    /// @brief The backend that creates and destroys the assets.
    using Owner = AssetTraits<Id>::Owner;

    Assets() = default;
    ~Assets() = default;
    Assets(const Assets& other) = delete;
    Assets& operator=(const Assets& other) = delete;
    Assets(Assets&& other) noexcept = default;
    Assets& operator=(Assets&& other) noexcept = default;

    /// @return The number of assets known.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Records an asset the backend already created.
    /// @return False, recording nothing, if the name or the id is already known.
    bool add(std::string name, Id id, Info info);

    /// @return The id of the asset with this name, or nothing if none.
    [[nodiscard]] std::optional<Id> find(std::string_view name) const noexcept;

    /// @return What is known about the asset with this id, or nullptr if none.
    [[nodiscard]] const Info* getInfo(Id id) const noexcept;

    /// @return The name of the asset with this id, or nothing if none.
    [[nodiscard]] std::optional<std::string_view> getName(Id id) const noexcept;

    /// @brief Asks the backend to destroy the asset with this name, and forgets it.
    /// @return False, doing nothing, if no asset has this name.
    bool unload(Owner& owner, std::string_view name);

  private:
    /// @brief One known asset.
    struct Entry {
        std::string name;  ///< Its name: its path, for a file.
        Id id;             ///< The id its backend gave it.
        Info info;         ///< What is known about it.
    };

    /// @return The entry with this name, or nullptr.
    [[nodiscard]] const Entry* findEntry(std::string_view name) const noexcept;

    /// @return The entry with this id, or nullptr.
    [[nodiscard]] const Entry* findEntry(Id id) const noexcept;

    std::vector<Entry> _entries;  ///< Every known asset, in the order they were added.
};
}  // namespace rtype::plugins::backend

#include "Assets.tpp"

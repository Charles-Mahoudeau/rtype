/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SlotMap
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace rtype::render::vulkan::core {

/// @brief A key a SlotMap accepts: a slot index and the generation of the slot it was given for.
template <class Key>
concept SlotKey = requires(Key key) {
    { key.index } -> std::convertible_to<std::uint32_t>;
    { key.generation } -> std::convertible_to<std::uint32_t>;
};

/// @brief Values reached through small copyable keys that detect use after removal: the renderer's textures behind
/// their TextureId.
///
/// @details A key is a slot index and the slot's generation. Removing a value bumps its slot's generation, then the
/// slot is reused by a later insert(): a key kept from before no longer matches, so find() returns null instead of the
/// new value. Generations start at 1, so a key with generation 0 (a default TextureId) never matches. Insertion, lookup
/// and removal are O(1).
/// @warning insert() may move the stored values: do not keep a pointer from find() across it.
/// @tparam Key An aggregate of `index` and `generation` (plugins::backend::graphics::TextureId).
/// @tparam Value Move-constructible.
template <SlotKey Key, class Value>
class SlotMap {
  public:
    SlotMap() = default;
    ~SlotMap() = default;
    SlotMap(const SlotMap&) = default;
    SlotMap& operator=(const SlotMap&) = default;
    SlotMap(SlotMap&&) noexcept = default;
    SlotMap& operator=(SlotMap&&) noexcept = default;

    /// @return The number of values stored.
    [[nodiscard]] std::size_t getSize() const noexcept { return _size; }

    /// @brief Stores @p value in a free slot, reusing a removed one first.
    /// @return Its key.
    [[nodiscard]] Key insert(Value value);

    /// @return The value of @p key, or null if it was removed or never inserted.
    [[nodiscard]] Value* find(Key key);
    /// @return The value of @p key, or null if it was removed or never inserted.
    [[nodiscard]] const Value* find(Key key) const;

    /// @brief Removes the value of @p key, which no longer matches afterwards.
    /// @return The value, for the caller to destroy when it can; empty if @p key matched none.
    std::optional<Value> erase(Key key);

  private:
    /// @brief One value, or a free place for one.
    struct Slot {
        std::optional<Value> value;   ///< Empty when the slot is free.
        std::uint32_t generation{1};  ///< Bumped when its value is removed; never 0.
    };

    /// @return The slot @p key names if its generation matches and it holds a value, null otherwise.
    [[nodiscard]] const Slot* slotOf(Key key) const;

    std::vector<Slot> _slots;               ///< Indexed by Key::index.
    std::vector<std::uint32_t> _freeSlots;  ///< Indices of free slots, the last freed reused first.
    std::size_t _size{0};                   ///< Number of slots holding a value.
};

}  // namespace rtype::render::vulkan::core

#include "SlotMap.tpp"

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Commands
*/

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <utility>

#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
template <Component T>
void Commands::add(Entity entity, T value) {
    const std::uint32_t column = columnOf<T>();
    storage::ByteColumn& values = std::next(_columns.begin(), column)->values;
    static_cast<void>(
        values.emplaceBackMove(std::as_writable_bytes(std::span{&value, 1}).first(values.getElementSize())));
    _records.push_back(Record{.kind = Kind::kAdd,
                              .entity = entity,
                              .column = column,
                              .index = static_cast<std::uint32_t>(values.getSize() - 1)});
}  // `value`, now moved-from, is destroyed here

template <Component T>
void Commands::remove(Entity entity) {
    _records.push_back(Record{.kind = Kind::kRemove, .entity = entity, .column = columnOf<T>(), .index = 0});
}

template <Component T, typename F>
    requires std::invocable<F&, T&> && std::copy_constructible<F>
void Commands::patch(Entity entity, F edit) {
    static_assert(!std::same_as<T, Parent> && !std::same_as<T, Children>,
                  "Parent and Children are changed through the World (setParent, removeParent)");
    recordPatch(entity, columnOf<T>(), [edit = std::move(edit)](std::span<std::byte> value) mutable {
        std::invoke(edit, *storage::viewAs<T>(value));
    });
}

template <Component T>
Commands::EntityCommands& Commands::EntityCommands::add(T value) {
    _commands->add(_entity, std::move(value));
    return *this;
}

template <Component T>
Commands::EntityCommands& Commands::EntityCommands::remove() {
    _commands->remove<T>(_entity);
    return *this;
}

template <Component T, typename F>
    requires std::invocable<F&, T&> && std::copy_constructible<F>
Commands::EntityCommands& Commands::EntityCommands::patch(F edit) {
    _commands->patch<T>(_entity, std::move(edit));
    return *this;
}

template <Component T>
std::uint32_t Commands::columnOf() {
    if (const std::optional<std::uint32_t> column = findColumn(TypeTraits<T>::kLayoutHash)) {
        return *column;
    }
    _columns.push_back(Column{.key = TypeTraits<T>::kLayoutHash,
                              .id = 0,
                              .resolve = &registerIfNeeded<T>,
                              .values = storage::ByteColumn{TypeTraits<T>::makeTypeInfo()}});
    return static_cast<std::uint32_t>(_columns.size() - 1);
}

template <Component T>
ComponentId Commands::registerIfNeeded(Components& components) {
    if (const ComponentInfo* info = components.getRegistry().find<T>()) {
        return info->type.id;  // keeps the options it was registered with
    }
    if constexpr (std::same_as<T, Parent>) {
        return components.registerType<T>(kParentOptions);
    } else {
        return components.registerType<T>();
    }
}
}  // namespace rtype::ecs

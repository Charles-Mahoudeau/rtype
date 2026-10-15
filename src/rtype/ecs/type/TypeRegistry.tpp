/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeRegistry
*/

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/type/NameHash.hpp"

namespace rtype::ecs {
template <typename Info>
std::size_t TypeRegistry<Info>::getCount() const noexcept {
    return _infos.size();
}

template <typename Info>
std::span<const TypeId> TypeRegistry<Info>::getReplicatedIds() const noexcept {
    return _replicatedIds;
}

template <typename Info>
Result<TypeId> TypeRegistry<Info>::registerType(Info info) {
    if (Result<void> valid = Traits::validate(info); !valid) {
        return tl::unexpected<Error>{std::move(valid.error())};
    }
    info.type.layoutHash = computeLayoutHash(info.type);
    const std::uint64_t nameHash = hashName(info.type.name);
    if (const Info* registered = findByHash(nameHash); registered != nullptr) {
        // Same name hash: the same type registered again, or a conflict.
        if (Result<void> same = detail::checkSameType(Traits::kLabel, registered->type, info.type); !same) {
            return tl::unexpected<Error>{std::move(same.error())};
        }
        if (Result<void> same = Traits::checkSameOptions(*registered, info); !same) {
            return tl::unexpected<Error>{std::move(same.error())};
        }
        return registered->type.id;
    }
    if (_infos.size() >= kInvalidTypeId) {
        return tl::unexpected<Error>{detail::noIdLeft(Traits::kLabel, info.type.name)};
    }
    const auto id = static_cast<TypeId>(_infos.size());
    info.type.id = id;
    const bool replicated = info.type.replication != ReplicationMode::kNone;
    _infos.push_back(std::move(info));
    _idsByHash.emplace(nameHash, id);
    if (replicated) {
        _replicatedIds.push_back(id);
    }
    return id;
}

template <typename Info>
template <OfKind<Info> T>
TypeId TypeRegistry<Info>::registerType(Options options) {
    Result<TypeId> id = registerType(Traits::template make<T>(options));
    if (!id) {
        throw exceptions::TypeRegistrationException{std::string{id.error().getMessage()}};
    }
    return *id;
}

template <typename Info>
const Info* TypeRegistry<Info>::findById(TypeId id) const noexcept {
    if (id >= _infos.size()) {
        return nullptr;
    }
    // In range: checked above, so the element is reached through an iterator rather than a throwing at().
    return &*std::next(_infos.begin(), static_cast<std::ptrdiff_t>(id));
}

template <typename Info>
const Info* TypeRegistry<Info>::findByName(std::string_view name) const noexcept {
    const Info* info = findByHash(hashName(name));
    return info != nullptr && info->type.name == name ? info : nullptr;
}

template <typename Info>
const Info* TypeRegistry<Info>::findByHash(std::uint64_t nameHash) const noexcept {
    const auto found = _idsByHash.find(nameHash);
    return found == _idsByHash.end() ? nullptr : findById(found->second);
}

template <typename Info>
template <OfKind<Info> T>
const Info* TypeRegistry<Info>::find() const noexcept {
    const Info* info = findByHash(TypeTraits<T>::kHash);
    // The name hash found a candidate; the name rules out a hash collision, the layout hash a different type
    // registered under the same name (its bytes would not be a T).
    if (info == nullptr || info->type.name != TypeTraits<T>::kName ||
        info->type.layoutHash != TypeTraits<T>::kLayoutHash) {
        return nullptr;
    }
    return info;
}
}  // namespace rtype::ecs

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineCall
*/

#include "EngineCall.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <format>
#include <new>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "rtype/scripting/view/FieldAccess.hpp"

namespace rtype::scripting::call {
namespace {
/// @brief The bytes of one value of a call data type, aligned as it requires.
class Bytes {
  public:
    /// @brief Allocates a value of `type`, or nothing when there is no type, built from its defaults.
    explicit Bytes(const std::optional<ecs::TypeInfo>& type) {
        if (!type || type->size == 0) {
            return;
        }
        _alignment = std::align_val_t{std::max<std::size_t>(type->alignment, 1)};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): released by the destructor, with the same alignment.
        _data = static_cast<std::byte*>(::operator new(type->size, _alignment));
        _size = type->size;
        std::fill_n(_data, _size, std::byte{0});
        if (type->hooks.construct != nullptr) {
            type->hooks.construct(_data);  // the defaults the type's members declare
        }
    }

    ~Bytes() {
        if (_data != nullptr) {
            ::operator delete(_data, _alignment);
        }
    }

    Bytes(const Bytes& other) = delete;
    Bytes& operator=(const Bytes& other) = delete;
    Bytes(Bytes&& other) = delete;
    Bytes& operator=(Bytes&& other) = delete;

    /// @return The bytes; empty when there is no value.
    [[nodiscard]] std::span<std::byte> get() const noexcept { return {_data, _size}; }

  private:
    std::byte* _data{nullptr};                               ///< The value, or null.
    std::size_t _size{0};                                    ///< Its size in bytes.
    std::align_val_t _alignment{alignof(std::max_align_t)};  ///< Its alignment, for the delete.
};

/// @return A Luau error naming the call.
[[nodiscard]] luau::exceptions::RuntimeError fail(const ecs::CallInfo& info, const std::string_view why) {
    return luau::exceptions::RuntimeError{std::format("engine call '{}': {}", info.name, why)};
}

/// @brief Writes the fields the script gave over the defaults of the arguments.
void fillArguments(const ecs::CallInfo& info, const luau::Value* fields, const std::span<std::byte> bytes) {
    if (fields == nullptr || fields->is<luau::Value::Nil>()) {
        return;
    }
    const auto* table = fields->tryAs<luau::Table>();
    if (table == nullptr) {
        throw fail(info, "expects a table of arguments");
    }
    if (!info.arguments) {
        const auto count = table->count();
        if (count && *count > 0) {
            throw fail(info, "takes no arguments");
        }
        return;
    }
    if (auto filled = view::fillFromTable(*info.arguments, *table, bytes); !filled) {
        throw fail(info, filled.error().message());
    }
}

/// @return The values a script gets for the result.
[[nodiscard]] luau::ValuePack readResult(const luau::Runtime& runtime, const ecs::CallInfo& info,
                                         const std::span<const std::byte> bytes) {
    luau::ValuePack pack;
    if (!info.result || info.result->fields.empty()) {
        return pack;
    }
    const view::FieldReadContext context{
        .runtime = &runtime,
        .entities = runtime.usertype<ecs::Entity>(),
        .assetIds = runtime.usertype<core::asset::AssetId>(),
    };
    const auto read = [&](const ecs::FieldInfo& field) {
        auto value = view::readField(info.result->name, field, bytes, context);
        if (!value) {
            throw fail(info, value.error().message());
        }
        return std::move(*value);
    };
    if (info.result->fields.size() == 1) {
        pack.add(read(info.result->fields.front()));
        return pack;
    }
    auto table = luau::Table::create(runtime, 0, static_cast<int>(info.result->fields.size()));
    if (!table) {
        throw fail(info, table.error().message());
    }
    for (const ecs::FieldInfo& field : info.result->fields) {
        if (auto set = table->set(field.name, read(field)); !set) {
            throw fail(info, set.error().message());
        }
    }
    pack.add(luau::Value{std::move(*table)});
    return pack;
}
}  // namespace

luau::ValuePack callEngine(const luau::Runtime& runtime, const ecs::CallInfo& info, const luau::Value* fields,
                           const Invoker& invoke) {
    const Bytes arguments{info.arguments};
    fillArguments(info, fields, arguments.get());
    const Bytes result{info.result};
    try {
        invoke(arguments.get(), result.get());
    } catch (const luau::exceptions::RuntimeError&) {
        throw;
    } catch (const std::exception& error) {
        throw fail(info, error.what());
    }
    return readResult(runtime, info, result.get());
}
}  // namespace rtype::scripting::call

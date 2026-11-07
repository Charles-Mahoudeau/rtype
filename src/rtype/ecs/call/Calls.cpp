/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Calls
*/

#include "Calls.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "core/hash/NameHash.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/exceptions/CallException.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace {
/// @return The size of the bytes of a described value: 0 when there is no description.
[[nodiscard]] std::size_t sizeOf(const std::optional<TypeInfo>& type) noexcept { return type ? type->size : 0; }
}  // namespace

Calls::~Calls() = default;

Calls::Calls(Calls&& other) noexcept = default;

Calls& Calls::operator=(Calls&& other) noexcept = default;

std::size_t Calls::getCount() const noexcept { return _calls.size(); }

CallId Calls::add(CallInfo info) {
    if (info.name.empty()) {
        throw exceptions::CallException{"An engine call needs a name"};
    }
    if (info.body == nullptr) {
        throw exceptions::CallException{std::format("Engine call '{}' has no function", info.name)};
    }
    const std::uint64_t hash = core::hash::hashName(info.name);
    if (const auto found = _idsByHash.find(hash); found != _idsByHash.end()) {
        const std::string& registered = _calls.at(found->second).name;
        if (registered == info.name) {
            throw exceptions::CallException{std::format("Engine call '{}' is already registered", info.name)};
        }
        throw exceptions::CallException{
            std::format("Engine call '{}' has the same name hash as '{}': rename one of them", info.name, registered)};
    }
    const auto id = static_cast<CallId>(_calls.size());
    if (id == kInvalidCallId) {
        throw exceptions::CallException{std::format("Engine call '{}': every id is taken", info.name)};
    }
    info.id = id;
    info.access = SystemAccess{};  // derived from the body, whatever the caller filled in
    info.body->addAccess(info.access);
    _calls.push_back(std::move(info));
    _idsByHash.emplace(hash, id);
    return id;
}

const CallInfo* Calls::findById(const CallId id) const noexcept {
    // Checked here: at() could throw from a noexcept function.
    return id < _calls.size() ? &*std::next(_calls.begin(), static_cast<std::ptrdiff_t>(id)) : nullptr;
}

const CallInfo* Calls::findByName(const std::string_view name) const noexcept {
    const auto found = _idsByHash.find(core::hash::hashName(name));
    if (found == _idsByHash.end()) {
        return nullptr;
    }
    const CallInfo& info =
        *std::next(_calls.begin(), static_cast<std::ptrdiff_t>(found->second));  // ids in the map are calls
    return info.name == name ? &info : nullptr;
}

void Calls::invoke(World& world, const CallId id, const std::span<const std::byte> arguments,
                   const std::span<std::byte> result) {
    if (id >= _calls.size()) {
        throw exceptions::CallException{std::format("No engine call has the id {}", id)};
    }
    CallInfo& info = _calls.at(id);
    checkCallBytes(info, arguments, result);
    Commands commands{world};
    const Tick thisRun = world.getChangeTick();
    // A body of its own for each invocation: the parameters are built as for a first run (an EventReader starts at 0).
    const std::unique_ptr<detail::CallBody> body = info.body->bind(world);
    body->invoke(
        SystemContext{.world = &world, .commands = &commands, .lastRun = thisRun - kMaxChangeAge, .thisRun = thisRun},
        arguments, result);
    commands.flush();
    world.advanceChangeTick();
}

void checkCallBytes(const CallInfo& info, const std::span<const std::byte> arguments,
                    const std::span<const std::byte> result) {
    if (arguments.size() != sizeOf(info.arguments)) {
        throw exceptions::CallException{std::format("Engine call '{}' takes {} bytes of arguments, not {}", info.name,
                                                    sizeOf(info.arguments), arguments.size())};
    }
    if (result.size() != sizeOf(info.result)) {
        throw exceptions::CallException{
            std::format("Engine call '{}' returns {} bytes, not {}", info.name, sizeOf(info.result), result.size())};
    }
}
}  // namespace rtype::ecs

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkInput
*/

#include "NetworkInput.hpp"

#include <glm/ext/vector_float2.hpp>
#include <string>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/plugins/input/Input.hpp"
#include "rtype/plugins/input/InputAction.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::engine {
namespace {
void fillNetworkInput(ecs::Res<plugins::input::Input> input, ecs::Res<replication::input::InputLayout> layout,
                      ecs::ResMut<replication::client::ClientState> client) {
    if (!layout->actions().empty()) {
        client->setInput(readNetworkInput(*input, *layout));
    }
}
}  // namespace

replication::component::PlayerInput readNetworkInput(const plugins::input::Input& input,
                                                     const replication::input::InputLayout& layout) {
    replication::component::PlayerInput read;
    for (const replication::input::InputAction& declared : layout.actions()) {
        const plugins::input::InputAction* action = input.findAction(declared.name);
        if (action == nullptr) {
            continue;
        }
        switch (declared.kind) {
            case replication::input::InputKind::kButton:
                layout.setButton(read, declared.name, action->isHeld());
                break;
            case replication::input::InputKind::kAxis:
                layout.setAxis(read, declared.name, action->readAxis());
                break;
            case replication::input::InputKind::kVector2: {
                const glm::vec2 vector = action->readVector();
                layout.setVector(read, declared.name, {vector.x, vector.y});
                break;
            }
        }
    }
    return read;
}

void addNetworkInput(ecs::App& app) {
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kNetworkInputSystem}, &fillNetworkInput)
        .before(std::string{replication::client::ClientPlugin::kSendInputSystem})
        .setSide(ecs::Side::kClient);
}
}  // namespace rtype::engine

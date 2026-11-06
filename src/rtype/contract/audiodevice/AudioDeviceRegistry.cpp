/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioDeviceRegistry
*/

#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rtype::contract::audiodevice {

std::vector<std::string> AudioDeviceRegistry::getNames() const {
    std::vector<std::string> names;
    names.reserve(_factories.size());
    for (const auto& [name, factory] : _factories) {
        names.push_back(name);
    }
    return names;
}

const AudioDeviceRegistry::Factory* AudioDeviceRegistry::find(std::string_view name) const {
    const auto found = _factories.find(name);
    return found != _factories.end() ? &found->second : nullptr;
}

void AudioDeviceRegistry::add(std::string name, Factory factory) {
    _factories.insert_or_assign(std::move(name), std::move(factory));
}

}  // namespace rtype::contract::audiodevice

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioAssets
*/

#include "plugins/audio/AudioAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/audio/exceptions/AudioExceptions.hpp"

namespace rtype::plugins::audio {

std::size_t AudioAssets::getCount() const noexcept { return _entries.size(); }

asset::AssetId AudioAssets::load(const std::filesystem::path& path, const SoundDesc& desc) {
    return request(path.lexically_normal().generic_string(), {}, desc);
}

asset::AssetId AudioAssets::load(const std::string& name, std::span<const std::byte> encoded, const SoundDesc& desc) {
    return request(std::filesystem::path{name}.lexically_normal().generic_string(), {encoded.begin(), encoded.end()},
                   desc);
}

const AudioAssets::Entry* AudioAssets::get(asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

std::optional<SoundId> AudioAssets::getSound(asset::AssetId id) const {
    const Entry* entry = get(id);
    return entry != nullptr && entry->state == SoundState::kLoaded ? std::optional{entry->sound} : std::nullopt;
}

bool AudioAssets::unload(asset::AssetId id) {
    const auto found = _entries.find(id);
    if (found == _entries.end()) {
        return false;
    }
    if (found->second.state == SoundState::kLoaded) {
        _unloaded.push_back(found->second.sound);
    }
    std::erase(_queued, id);
    _entries.erase(found);
    return true;
}

const std::vector<asset::AssetId>& AudioAssets::getQueued() const noexcept { return _queued; }

void AudioAssets::setLoaded(asset::AssetId id, SoundId sound) {
    if (Entry* entry = dequeue(id)) {
        entry->state = SoundState::kLoaded;
        entry->sound = sound;
        entry->encoded = {};
    }
}

void AudioAssets::setFailed(asset::AssetId id, std::string error) {
    if (Entry* entry = dequeue(id)) {
        entry->state = SoundState::kFailed;
        entry->error = std::move(error);
        entry->encoded = {};
    }
}

std::vector<SoundId> AudioAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

asset::AssetId AudioAssets::request(std::string path, std::vector<std::byte> encoded, const SoundDesc& desc) {
    const asset::AssetId id = asset::AssetId::fromPath(path);
    if (const Entry* known = get(id)) {
        if (known->path != path) {
            throw exceptions::AudioLoadException{
                std::format("Sound '{}' has the same id as '{}': rename one of them", path, known->path)};
        }
        return id;
    }
    _entries.emplace(id, Entry{.path = std::move(path), .desc = desc, .encoded = std::move(encoded)});
    _queued.push_back(id);
    return id;
}

AudioAssets::Entry* AudioAssets::dequeue(asset::AssetId id) {
    const auto queued = std::ranges::find(_queued, id);
    if (queued == _queued.end()) {
        return nullptr;
    }
    _queued.erase(queued);
    return &_entries.at(id);
}

void updateSounds(AudioAssets& assets, IAudioDevice& device) {
    for (const SoundId sound : assets.takeUnloaded()) {
        device.destroySound(sound);
    }
    const std::vector<asset::AssetId> queued = assets.getQueued();
    for (const asset::AssetId id : queued) {
        const AudioAssets::Entry& entry = *assets.get(id);
        try {
            const SoundId sound = entry.encoded.empty()
                                      ? device.createSound(std::filesystem::path{entry.path}, entry.desc)
                                      : device.createSound(entry.encoded, entry.desc);
            assets.setLoaded(id, sound);
        } catch (const std::exception& error) {
            std::cerr << "[AudioAssets] " << error.what() << '\n';
            assets.setFailed(id, error.what());
        }
    }
}
}  // namespace rtype::plugins::audio

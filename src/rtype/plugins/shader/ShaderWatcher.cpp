/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderWatcher
*/

#include "plugins/shader/ShaderWatcher.hpp"

#include <filesystem>
#include <system_error>
#include <utility>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/shader/ShaderAssets.hpp"

namespace rtype::plugins::shader {

ShaderWatcher::ShaderWatcher(std::filesystem::path assetDirectory)
    : _assetDirectory{std::move(assetDirectory)}, _files{scan()}, _nextPoll{Clock::now() + kInterval} {}

void ShaderWatcher::update(ShaderAssets& shaders, const Clock::time_point now) {
    if (now < _nextPoll) {
        return;
    }
    _nextPoll = now + kInterval;
    poll(shaders);
}

void ShaderWatcher::poll(ShaderAssets& shaders) {
    WriteTimes files = scan();
    for (const auto& [path, time] : files) {
        const auto known = _files.find(path);
        if (known == _files.end() || known->second != time) {
            static_cast<void>(shaders.reload(core::asset::AssetId::fromPath(path)));
        }
    }
    _files = std::move(files);
}

ShaderWatcher::WriteTimes ShaderWatcher::scan() const {
    WriteTimes files;
    std::error_code error;
    std::filesystem::recursive_directory_iterator entry{
        _assetDirectory / kDirectory, std::filesystem::directory_options::skip_permission_denied, error};
    const std::filesystem::recursive_directory_iterator end;
    while (!error && entry != end) {
        std::error_code fileError;
        const auto time = std::filesystem::last_write_time(entry->path(), fileError);
        if (!fileError && entry->is_regular_file(fileError)) {
            files.emplace(core::asset::normalizePath(entry->path().lexically_relative(_assetDirectory)), time);
        }
        entry.increment(error);
    }
    return files;
}

}  // namespace rtype::plugins::shader

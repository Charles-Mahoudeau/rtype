/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleFiles
*/

#include "CapsuleFiles.hpp"

#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "rtype/ecs/component/Components.hpp"

namespace rtype::plugins::capsule {
CapsuleFiles::CapsuleFiles(std::filesystem::path assetDirectory, const ecs::Components& components)
    : _directory{std::move(assetDirectory)}, _components{&components}, _nextPoll{Clock::now() + kInterval} {}

const std::filesystem::path& CapsuleFiles::getDirectory() const noexcept { return _directory; }

std::vector<CapsuleId> CapsuleFiles::loadAll(Capsules& capsules) {
    _files = scan();
    std::set<std::string> pending;
    for (const auto& [path, time] : _files) {
        pending.insert(path);
    }
    std::vector<CapsuleId> read;
    for (const auto& [path, time] : _files) {
        if (pending.contains(path)) {
            this->read(capsules, path, pending, read);
        }
    }
    return read;
}

std::vector<CapsuleId> CapsuleFiles::update(Capsules& capsules, Clock::time_point now) {
    if (now < _nextPoll) {
        return {};
    }
    _nextPoll = now + kInterval;
    WriteTimes files = scan();
    std::set<std::string> pending;
    for (const auto& [path, time] : files) {
        const auto known = _files.find(path);
        if (known == _files.end() || known->second != time) {
            pending.insert(path);
        }
    }
    _files = std::move(files);
    for (bool grew = !pending.empty(); grew;) {  // and the files that instance them, at any depth
        grew = false;
        for (const auto& [path, uses] : _uses) {
            if (pending.contains(path)) {
                continue;
            }
            for (const std::string& used : uses) {
                if (pending.contains(used)) {
                    grew = pending.insert(path).second || grew;
                    break;
                }
            }
        }
    }
    std::vector<CapsuleId> reread;
    while (!pending.empty()) {
        const std::string path = *pending.begin();
        try {
            read(capsules, path, pending, reread);
        } catch (const std::exception& error) {  // the old version stays: fix the file and save it again
            pending.erase(path);
            std::cerr << std::format("[capsule] '{}' kept as it was: {}\n", path, error.what());
        }
    }
    return reread;
}

CapsuleFiles::WriteTimes CapsuleFiles::scan() const {
    WriteTimes files;
    std::error_code error;
    std::filesystem::recursive_directory_iterator entry{
        _directory, std::filesystem::directory_options::skip_permission_denied, error};
    const std::filesystem::recursive_directory_iterator end;
    while (!error && entry != end) {
        std::error_code fileError;
        const auto time = std::filesystem::last_write_time(entry->path(), fileError);
        if (!fileError && entry->is_regular_file(fileError) && format::isCapsuleFile(entry->path())) {
            files.emplace(core::asset::normalizePath(entry->path().lexically_relative(_directory)), time);
        }
        entry.increment(error);
    }
    return files;
}

// NOLINTNEXTLINE(misc-no-recursion): each file is read once per pass, `pending` stops a file that instances itself.
void CapsuleFiles::read(Capsules& capsules, const std::string& path, std::set<std::string>& pending,
                        std::vector<CapsuleId>& read) {
    pending.erase(path);
    std::string text;
    try {
        text = core::asset::readTextFile(core::asset::resolveAssetPath(_directory, path));
    } catch (const core::asset::exceptions::AssetLoadException& error) {
        throw exceptions::CapsuleException{std::format("{}: {}", path, error.what())};
    }
    std::set<std::string> uses;
    Capsule capsule = format::readCapsule(path, text, *_components, capsules, [&](std::string_view used) {
        const std::string instanced{used};
        uses.insert(instanced);
        if (pending.contains(instanced)) {
            // NOLINTNEXTLINE(misc-no-recursion): the instanced file's values are the base of this file's overrides
            this->read(capsules, instanced, pending, read);
        }
    });
    read.push_back(capsules.define(std::move(capsule)).getId());
    _uses.insert_or_assign(path, std::move(uses));
}
}  // namespace rtype::plugins::capsule

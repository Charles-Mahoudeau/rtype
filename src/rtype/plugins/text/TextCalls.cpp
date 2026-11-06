/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextCalls
*/

#include "plugins/text/TextCalls.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

#include "plugins/text/FontAssets.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::text {
namespace {
FontResult loadFont(ecs::ResMut<FontAssets> fonts, const LoadFontArguments& arguments) {
    return FontResult{.font = fonts->load(std::filesystem::path{arguments.path.view()})};
}
}  // namespace

void addTextForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "TextForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addTextForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addTextCalls(ecs::App& app) {
    app.addCall(std::string{kLoadFontCall}, &loadFont);
    addTextForms(app.getPluginRegistry());
}

}  // namespace rtype::plugins::text

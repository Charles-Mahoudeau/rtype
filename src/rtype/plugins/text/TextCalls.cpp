/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextCalls
*/

#include "plugins/text/TextCalls.hpp"

#include <filesystem>
#include <string>

#include "plugins/text/FontAssets.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::text {
namespace {
FontResult loadFont(ecs::ResMut<FontAssets> fonts, const LoadFontArguments& arguments) {
    return FontResult{.font = fonts->load(std::filesystem::path{arguments.path.view()})};
}
}  // namespace

void addTextCalls(ecs::App& app) { app.addCall(std::string{kLoadFontCall}, &loadFont); }

}  // namespace rtype::plugins::text

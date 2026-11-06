/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetForms
*/

#include "AssetForms.hpp"

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "FormTable.hpp"
#include "SetupCalls.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace rtype::scripting::setup {
namespace {
// The calls of the asset plugins, as they name them (scripting does not depend on the plugins).
constexpr std::string_view kLoadTextureCall = "rtype.backend.loadTexture";
constexpr std::string_view kLoadSoundCall = "rtype.audio.loadSound";
constexpr std::string_view kLoadFontCall = "rtype.text.loadFont";
constexpr std::string_view kLoadTilemapCall = "rtype.tilemap.load";
constexpr std::string_view kLoadAtlasCall = "rtype.sprite.loadAtlas";
constexpr std::string_view kCreateAtlasCall = "rtype.sprite.createAtlas";
constexpr std::string_view kAddFrameCall = "rtype.sprite.addFrame";
constexpr std::string_view kAddAnimationCall = "rtype.sprite.addAnimation";
constexpr std::string_view kLoadShaderCall = "rtype.shader.load";
constexpr std::string_view kAddMaterialCall = "rtype.shader.addMaterial";
constexpr std::string_view kSetMaterialValueCall = "rtype.shader.setMaterialValue";
constexpr std::string_view kSetMaterialTextureCall = "rtype.shader.setMaterialTexture";

constexpr std::string_view kAtlasForm = "engine.assets.atlas";
constexpr std::string_view kMaterialForm = "engine.assets.material";

constexpr std::array<std::string_view, 2> kTextureOptions{"linear", "wrap"};
constexpr std::array<std::string_view, 1> kSoundOptions{"stream"};
constexpr std::array<std::string_view, 7> kAtlasKeys{
    "path", "texture", "name", "linear", "grid", "frames", "animations",
};
constexpr std::array<std::string_view, 5> kGridKeys{"frameSize", "count", "spacing", "columns", "offset"};
constexpr std::array<std::string_view, 4> kFrameKeys{"name", "position", "size", "origin"};
constexpr std::array<std::string_view, 6> kAnimationKeys{
    "frames", "first", "count", "frameDuration", "direction", "plays",
};
constexpr std::array<std::string_view, 4> kMaterialKeys{"name", "shader", "values", "textures"};
constexpr std::array<std::string_view, 4> kVectorKeys{"x", "y", "z", "w"};

/// @brief A form that asks for one file: `texture(path, options)`.
struct FileForm {
    std::string_view function;                  ///< Its name in `engine.assets`.
    std::string_view form;                      ///< Its full name, for the messages.
    std::string_view call;                      ///< The call that loads the file.
    std::span<const std::string_view> options;  ///< Its boolean options, passed on as they are named.
};

constexpr std::array kFileForms{
    FileForm{
        .function = "texture",
        .form = "engine.assets.texture",
        .call = kLoadTextureCall,
        .options = kTextureOptions,
    },
    FileForm{.function = "sound", .form = "engine.assets.sound", .call = kLoadSoundCall, .options = kSoundOptions},
    FileForm{.function = "font", .form = "engine.assets.font", .call = kLoadFontCall, .options = {}},
    FileForm{.function = "tilemap", .form = "engine.assets.tilemap", .call = kLoadTilemapCall, .options = {}},
    FileForm{.function = "shader", .form = "engine.assets.shader", .call = kLoadShaderCall, .options = {}},
};

/// @brief The calls an atlas form makes, read and checked before any is made.
struct AtlasPlan {
    std::string_view create;             ///< `rtype.sprite.loadAtlas` or `rtype.sprite.createAtlas`.
    CallFields atlas;                    ///< Its fields.
    std::string id;                      ///< The path or the name the atlas's id is the hash of.
    std::vector<CallFields> frames;      ///< The fields of each `rtype.sprite.addFrame`, but `atlas`.
    std::vector<CallFields> animations;  ///< The fields of each `rtype.sprite.addAnimation`, but `atlas`.
};

/// @return A pack of one value.
[[nodiscard]] luau::ValuePack packOf(luau::Value value) {
    luau::ValuePack pack;
    pack.add(std::move(value));
    return pack;
}

/// @brief Adds the `{x, y}` value of @p key to @p fields, if @p table has it.
void addVec2(const FormTable& table, const std::string_view key, CallFields& fields) {
    if (auto value = table.vec2(key)) {
        fields.emplace_back(key, std::move(*value));
    }
}

/// @brief Adds the whole number of @p key to @p fields, if @p table has it.
void addCount(const FormTable& table, const std::string_view key, CallFields& fields) {
    if (const auto value = table.count(key)) {
        fields.emplace_back(key, luau::Value{static_cast<double>(*value)});
    }
}

luau::ValuePack loadFile(const SetupCalls& calls, const FileForm& form, const luau::CallContext& context) {
    checkArgumentCount(form.form, context, 2);
    const luau::Value path = argumentAt(context, 0);
    const auto* text = path.tryAs<std::string>();
    if (text == nullptr || text->empty()) {
        throw luau::exceptions::RuntimeError{std::format("{}: expected the file's path first", form.form)};
    }
    const FormTable options =
        FormTable::optional(std::format("{}: options", form.form), argumentAt(context, 1), form.options);
    CallFields fields{{"path", path}};
    for (const std::string_view option : form.options) {
        if (const auto flag = options.boolean(option)) {
            fields.emplace_back(option, luau::Value{*flag});
        }
    }
    if (!calls.has(form.call)) {
        return packOf(calls.assetId(*text));
    }
    return calls.invoke(form.form, form.call, fields);
}

/// @brief Adds the fields of `grid` to the fields of `rtype.sprite.createAtlas`.
void readGrid(const luau::Value& value, CallFields& fields) {
    const FormTable grid{std::format("{}: 'grid'", kAtlasForm), value, kGridKeys};
    auto frameSize = grid.vec2("frameSize");
    const auto count = grid.count("count");
    if (!frameSize || !count || *count == 0) {
        grid.fail("expected a 'frameSize' and a 'count' of 1 or more");
    }
    fields.emplace_back("frameSize", std::move(*frameSize));
    fields.emplace_back("count", luau::Value{static_cast<double>(*count)});
    addVec2(grid, "spacing", fields);
    addCount(grid, "columns", fields);
    addVec2(grid, "offset", fields);
}

/// @return The fields of `rtype.sprite.addFrame` for the frame @p value, the @p number th of `frames`.
[[nodiscard]] CallFields readFrame(const std::size_t number, const luau::Value& value) {
    const FormTable frame{std::format("{}: frame {}", kAtlasForm, number), value, kFrameKeys};
    if (!frame.has("position") || !frame.has("size")) {
        frame.fail("expected a 'position' and a 'size'");
    }
    CallFields fields;
    if (auto name = frame.string("name")) {
        fields.emplace_back("name", luau::Value{std::move(*name)});
    }
    addVec2(frame, "position", fields);
    addVec2(frame, "size", fields);
    addVec2(frame, "origin", fields);
    return fields;
}

/// @brief Adds `first` and `count` to @p fields from the list of frame indices of @p animation, which must follow
/// each other.
void readFrameList(const FormTable& animation, std::span<const luau::Value> frames, CallFields& fields) {
    if (animation.has("first") || animation.has("count")) {
        animation.fail("expected either 'frames' or 'first' and 'count'");
    }
    if (frames.empty()) {
        animation.fail("'frames' is empty");
    }
    const auto first = frames.front().toUInt32();
    if (!first) {
        animation.fail("'frames' must list frame indices: whole numbers, 0 or more");
    }
    std::size_t offset = 0;
    for (const luau::Value& item : frames) {
        const auto frame = item.toUInt32();
        if (!frame || *frame != *first + offset) {
            animation.fail("'frames' must list frame indices that follow each other, such as {0, 1, 2}");
        }
        ++offset;
    }
    fields.emplace_back("first", luau::Value{static_cast<double>(*first)});
    fields.emplace_back("count", luau::Value{static_cast<double>(frames.size())});
}

/// @return The fields of `rtype.sprite.addAnimation` for the animation @p name.
[[nodiscard]] CallFields readAnimation(const std::string& name, const luau::Value& value) {
    const FormTable animation{std::format("{}: animation '{}'", kAtlasForm, name), value, kAnimationKeys};
    CallFields fields{{"name", luau::Value{name}}};
    if (const auto frames = animation.list("frames")) {
        readFrameList(animation, *frames, fields);
    } else {
        addCount(animation, "first", fields);
        addCount(animation, "count", fields);
    }
    if (const auto duration = animation.number("frameDuration")) {
        fields.emplace_back("frameDuration", luau::Value{*duration});
    }
    if (auto direction = animation.string("direction")) {
        fields.emplace_back("direction", luau::Value{std::move(*direction)});
    }
    addCount(animation, "plays", fields);
    return fields;
}

/// @return What an atlas form does, from its table.
[[nodiscard]] AtlasPlan readAtlas(const luau::Value& value) {
    const FormTable spec{std::string{kAtlasForm}, value, kAtlasKeys};
    const auto path = spec.string("path");
    const auto texture = spec.string("texture");
    if (path.has_value() == texture.has_value()) {
        spec.fail("expected either a 'path' (an atlas file) or a 'texture' (an atlas made from an image)");
    }
    if (path && (spec.has("name") || spec.has("grid") || spec.has("frames"))) {
        spec.fail("'name', 'grid' and 'frames' make an atlas from a 'texture', not from a 'path'");
    }
    AtlasPlan plan;
    if (path) {
        plan.create = kLoadAtlasCall;
        plan.atlas.emplace_back("path", luau::Value{*path});
        plan.id = *path;
    } else {
        const auto name = spec.string("name");
        plan.create = kCreateAtlasCall;
        plan.atlas.emplace_back("texture", luau::Value{*texture});
        if (name) {
            plan.atlas.emplace_back("name", luau::Value{*name});
        }
        plan.id = name.value_or(*texture);
        if (const luau::Value* grid = spec.find("grid")) {
            readGrid(*grid, plan.atlas);
        }
        const std::vector<luau::Value> frames = spec.list("frames").value_or(std::vector<luau::Value>{});
        for (std::size_t index = 0; index < frames.size(); ++index) {
            plan.frames.push_back(readFrame(index + 1, frames.at(index)));
        }
    }
    if (const auto linear = spec.boolean("linear")) {
        plan.atlas.emplace_back("linear", luau::Value{*linear});
    }
    if (const luau::Value* animations = spec.find("animations")) {
        for (const auto& [name, animation] :
             readNamedEntries(std::format("{}: 'animations'", kAtlasForm), *animations)) {
            plan.animations.push_back(readAnimation(name, animation));
        }
    }
    return plan;
}

/// @brief Makes the calls that fill an atlas: one per frame or animation, with the atlas's id.
void fillAtlas(const SetupCalls& calls, const luau::Value& atlas, const std::string_view call,
               const std::vector<CallFields>& items) {
    for (const CallFields& item : items) {
        CallFields fields{{"atlas", atlas}};
        fields.insert(fields.end(), item.begin(), item.end());
        static_cast<void>(calls.invoke(kAtlasForm, call, fields));
    }
}

luau::ValuePack atlas(const SetupCalls& calls, const luau::CallContext& context) {
    checkArgumentCount(kAtlasForm, context, 1);
    const AtlasPlan plan = readAtlas(argumentAt(context, 0));
    if (!calls.has(plan.create)) {
        return packOf(calls.assetId(plan.id));
    }
    luau::ValuePack result = calls.invoke(kAtlasForm, plan.create, plan.atlas);
    if (result.size() != 1) {
        throw luau::exceptions::RuntimeError{std::format("{}: '{}' gave no atlas", kAtlasForm, plan.create)};
    }
    const luau::Value& id = result.values().front();
    fillAtlas(calls, id, kAddFrameCall, plan.frames);
    fillAtlas(calls, id, kAddAnimationCall, plan.animations);
    return result;
}

/// @return The fields of `rtype.shader.setMaterialValue` for the value @p value of the `uniform` @p name: a number, or
/// a vector `{x, y}`, `{x, y, z}` or `{x, y, z, w}`.
[[nodiscard]] CallFields readMaterialValue(const std::string& name, const luau::Value& value) {
    CallFields fields{{"name", luau::Value{name}}};
    if (const auto* number = value.tryAs<double>()) {
        fields.emplace_back("x", luau::Value{*number});
        return fields;
    }
    const FormTable vector{std::format("{}: value '{}'", kMaterialForm, name), value, kVectorKeys};
    std::size_t size = 0;
    for (const std::string_view key : kVectorKeys) {
        const std::optional<double> component = vector.number(key);
        if (!component) {
            break;
        }
        fields.emplace_back(key, luau::Value{*component});
        ++size;
    }
    if (size < 2 || size != vector.size()) {
        vector.fail("expected a number, or a vector {x, y}, {x, y, z} or {x, y, z, w}");
    }
    fields.emplace_back("size", luau::Value{static_cast<double>(size)});
    return fields;
}

/// @brief Makes the calls that fill a material: one per value or texture, with the material's id.
void fillMaterial(const SetupCalls& calls, const luau::Value& material, const std::string_view call,
                  const std::vector<CallFields>& items) {
    for (const CallFields& item : items) {
        CallFields fields{{"material", material}};
        fields.insert(fields.end(), item.begin(), item.end());
        static_cast<void>(calls.invoke(kMaterialForm, call, fields));
    }
}

luau::ValuePack material(const SetupCalls& calls, const luau::CallContext& context) {
    checkArgumentCount(kMaterialForm, context, 1);
    const FormTable spec{std::string{kMaterialForm}, argumentAt(context, 0), kMaterialKeys};
    const std::optional<std::string> name = spec.string("name");
    const luau::Value* shader = spec.find("shader");
    if (!name || name->empty() || shader == nullptr) {
        spec.fail("expected a 'name' and a 'shader'");
    }
    std::vector<CallFields> values;
    if (const luau::Value* table = spec.find("values")) {
        for (const auto& [uniform, value] : readNamedEntries(std::format("{}: 'values'", kMaterialForm), *table)) {
            values.push_back(readMaterialValue(uniform, value));
        }
    }
    std::vector<CallFields> textures;
    if (const luau::Value* table = spec.find("textures")) {
        for (const auto& [uniform, texture] : readNamedEntries(std::format("{}: 'textures'", kMaterialForm), *table)) {
            textures.push_back(CallFields{{"name", luau::Value{uniform}}, {"texture", texture}});
        }
    }
    if (!calls.has(kAddMaterialCall)) {
        return packOf(calls.assetId(*name));
    }
    luau::ValuePack result =
        calls.invoke(kMaterialForm, kAddMaterialCall, CallFields{{"name", luau::Value{*name}}, {"shader", *shader}});
    if (result.values().empty()) {
        throw luau::exceptions::RuntimeError{std::format("{}: '{}' gave no material", kMaterialForm, kAddMaterialCall)};
    }
    const luau::Value& id = result.values().front();
    fillMaterial(calls, id, kSetMaterialValueCall, values);
    fillMaterial(calls, id, kSetMaterialTextureCall, textures);
    return result;
}
}  // namespace

luau::Library buildAssetForms(luau::Runtime& runtime, ecs::App& app) {
    const SetupCalls calls{runtime, app};
    luau::LibraryBuilder builder{runtime};
    builder.setName("assets");
    for (const FileForm& form : kFileForms) {
        builder.function(std::string{form.function}).setHandler([calls, form](const luau::CallContext& context) {
            return loadFile(calls, form, context);
        });
    }
    builder.function("atlas").setHandler([calls](const luau::CallContext& context) { return atlas(calls, context); });
    builder.function("material").setHandler([calls](const luau::CallContext& context) {
        return material(calls, context);
    });
    return builder.build();
}
}  // namespace rtype::scripting::setup

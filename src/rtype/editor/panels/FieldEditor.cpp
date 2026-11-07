/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldEditor
*/

#include "FieldEditor.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
constexpr float kDragSpeed = 0.1F;  ///< How much a float changes per pixel of a drag.
constexpr std::array<std::string_view, 5> kImageExtensions{".png", ".jpg", ".jpeg", ".bmp", ".tga"};

/// @brief The bytes of one value of a field, copied out of the value so a widget can change them.
using Scratch = std::array<std::byte, 256>;

/// @brief Shows a line of text.
void line(const std::string& text) { ImGui::TextUnformatted(text.c_str()); }

/// @brief Shows an integer of type @p type, held in @p value.
bool editInteger(const char* label, ecs::FieldType type, void* value) {
    switch (type) {
        case ecs::FieldType::kI8:
            return ImGui::DragScalar(label, ImGuiDataType_S8, value);
        case ecs::FieldType::kI16:
            return ImGui::DragScalar(label, ImGuiDataType_S16, value);
        case ecs::FieldType::kI32:
            return ImGui::DragScalar(label, ImGuiDataType_S32, value);
        case ecs::FieldType::kI64:
            return ImGui::DragScalar(label, ImGuiDataType_S64, value);
        case ecs::FieldType::kU8:
            return ImGui::DragScalar(label, ImGuiDataType_U8, value);
        case ecs::FieldType::kU16:
            return ImGui::DragScalar(label, ImGuiDataType_U16, value);
        case ecs::FieldType::kU32:
            return ImGui::DragScalar(label, ImGuiDataType_U32, value);
        case ecs::FieldType::kU64:
            return ImGui::DragScalar(label, ImGuiDataType_U64, value);
        default:
            return false;
    }
}

/// @return The path an asset id was read from, if the capsules know it.
std::string assetLabel(const ecs::World& world, std::uint64_t id) {
    if (id == 0) {
        return "(none)";
    }
    if (const auto* capsules = world.getResources().get<plugins::capsule::Capsules>()) {
        if (const std::string* path = capsules->findAssetPath(core::asset::AssetId{.value = id})) {
            return *path;
        }
    }
    if (const auto* effects = world.getResources().get<plugins::particle::ParticleEffectAssets>()) {
        if (const auto* entry = effects->getEntry(core::asset::AssetId{.value = id})) {
            return entry->path;  // a particle effect, known by the name a script gave it
        }
    }
    return std::format("{:016x}", id);
}

/// @brief Shows an asset field, and takes a file dropped from the Assets panel.
bool editAsset(ecs::World& world, const char* label, std::byte* value) {
    std::uint64_t id = 0;
    std::memcpy(&id, value, sizeof(id));
    const std::string shown = std::format("{}: {}", label, assetLabel(world, id));
    ImGui::Selectable(shown.c_str(), false);
    if (!ImGui::BeginDragDropTarget()) {
        return false;
    }
    bool changed = false;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetPayload)) {
        const std::string path = core::asset::normalizePath(std::filesystem::path{
            std::string{static_cast<const char*>(payload->Data), static_cast<std::size_t>(payload->DataSize)}});
        core::asset::AssetId asset = core::asset::AssetId::fromPath(path);
        std::string extension = std::filesystem::path{path}.extension().string();
        std::ranges::transform(extension, extension.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        auto textures = world.getResources().getMut<plugins::graphics::TextureAssets>();
        if (textures && std::ranges::find(kImageExtensions, extension) != kImageExtensions.end()) {
            asset = textures->load(path);  // queued: the renderer gets it before the next frame is drawn
        }
        if (auto capsules = world.getResources().getMut<plugins::capsule::Capsules>()) {
            capsules->rememberAssetPath(asset, path);  // so a saved scene names it by its path
        }
        std::memcpy(value, &asset.value, sizeof(asset.value));
        changed = true;
    }
    ImGui::EndDragDropTarget();
    return changed;
}

/// @brief Shows the value of a field with the widget of its type.
/// @param value Where the value is: the widget changes it in place.
/// @return Whether the user changed it.
bool editValue(ecs::World& world, const char* label, ecs::FieldType type, std::byte* value) {
    // The widgets of ImGui take the typed pointer of the value; the scratch bytes are aligned for any field type.
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    switch (type) {
        case ecs::FieldType::kBool:
            return ImGui::Checkbox(label, reinterpret_cast<bool*>(value));
        case ecs::FieldType::kF32:
            return ImGui::DragFloat(label, reinterpret_cast<float*>(value), kDragSpeed);
        case ecs::FieldType::kF64:
            return ImGui::DragScalar(label, ImGuiDataType_Double, value, kDragSpeed);
        case ecs::FieldType::kVec2:
            return ImGui::DragFloat2(label, reinterpret_cast<float*>(value), kDragSpeed);
        case ecs::FieldType::kVec3:
            return ImGui::DragFloat3(label, reinterpret_cast<float*>(value), kDragSpeed);
        case ecs::FieldType::kVec4:
            return ImGui::DragFloat4(label, reinterpret_cast<float*>(value), kDragSpeed);
        case ecs::FieldType::kEntity: {
            std::uint64_t bits = 0;
            std::memcpy(&bits, value, sizeof(bits));
            const ecs::Entity target{static_cast<std::uint32_t>(bits), static_cast<std::uint32_t>(bits >> 32U)};
            line(std::format("{}: {}", label, target.isNull() ? "null" : std::format("#{}", target.getIndex())));
            return false;
        }
        case ecs::FieldType::kAssetId:
            return editAsset(world, label, value);
        case ecs::FieldType::kString: {
            ecs::String text;
            std::memcpy(&text, value, sizeof(text));
            std::array<char, ecs::String::kCapacity + 1> buffer{};
            std::memcpy(buffer.data(), text.view().data(), text.size());
            if (!ImGui::InputText(label, buffer.data(), buffer.size())) {
                return false;
            }
            text.assign(buffer.data());
            std::memcpy(value, &text, sizeof(text));
            return true;
        }
        case ecs::FieldType::kText:
            return false;  // The bytes of a text are shown whole, by the list that holds them.
        default:
            return editInteger(label, type, value);
    }
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

/// @brief Shows a list field: a text as text, any other list as its length. Lists are not edited.
void showList(const ecs::FieldInfo& field, std::span<const std::byte> value) {
    const std::span<const std::byte> elements = field.list->elements(value.subspan(field.offset).data());
    if (field.type == ecs::FieldType::kText) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the bytes of a text are its characters.
        const std::string_view text{reinterpret_cast<const char*>(elements.data()), elements.size()};
        line(std::format("{}: \"{}\"", field.name, text));
        return;
    }
    line(std::format("{}: {} bytes of values", field.name, elements.size()));
}
}  // namespace

DefaultValue::DefaultValue(const ecs::TypeInfo& type)
    : _type{&type}, _storage(((type.size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t)) + 1) {
    assert(type.alignment <= alignof(std::max_align_t));
    if (_type->hooks.construct != nullptr) {
        _type->hooks.construct(_storage.data());
    }
}

DefaultValue::~DefaultValue() {
    if (_type->hooks.destruct != nullptr) {
        _type->hooks.destruct(_storage.data());
    }
}

std::span<const std::byte> DefaultValue::getBytes() const noexcept {
    return std::as_bytes(std::span{_storage}).first(_type->size);
}

bool isHiddenComponent(std::string_view name, bool inScene) noexcept {
    if (name == "rtype.Parent" || name == "rtype.Children" || name == "rtype.CapsuleInstance") {
        return true;
    }
    return inScene && (name == "rtype.Name" || name == "rtype.GlobalTransform");
}

bool editFields(ecs::World& world, const ecs::TypeInfo& type, std::span<std::byte> value) {
    bool changed = false;
    for (const ecs::FieldInfo& field : type.fields) {
        if (field.list != nullptr) {
            showList(field, value);
            continue;
        }
        const std::uint32_t size = ecs::getFieldLayout(field.type).size;
        for (std::uint32_t element = 0; element < field.count; ++element) {
            const std::string label = field.count > 1 ? std::format("{}[{}]", field.name, element) : field.name;
            const std::size_t offset = field.offset + (static_cast<std::size_t>(element) * size);
            alignas(std::max_align_t) Scratch scratch{};
            std::memcpy(scratch.data(), value.subspan(offset, size).data(), size);
            ImGui::PushID(static_cast<int>(offset));
            if (editValue(world, label.c_str(), field.type, scratch.data())) {
                std::memcpy(value.subspan(offset, size).data(), scratch.data(), size);
                changed = true;
            }
            ImGui::PopID();
        }
    }
    return changed;
}

}  // namespace rtype::editor::panels

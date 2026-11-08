/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetKind
*/

#include "AssetKind.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace rtype::editor::panels {

namespace {
using Extension = std::pair<std::string_view, AssetKind>;  ///< An extension in lower case, with its dot, and its kind.

constexpr std::array kExtensions = std::to_array<Extension>({
    {".scene", AssetKind::kScene}, {".actor", AssetKind::kActor}, {".capsule", AssetKind::kCapsule},
    {".luau", AssetKind::kScript}, {".lua", AssetKind::kScript},  {".png", AssetKind::kImage},
    {".jpg", AssetKind::kImage},   {".jpeg", AssetKind::kImage},  {".gif", AssetKind::kImage},
    {".bmp", AssetKind::kImage},   {".tga", AssetKind::kImage},   {".mp3", AssetKind::kAudio},
    {".ogg", AssetKind::kAudio},   {".wav", AssetKind::kAudio},   {".flac", AssetKind::kAudio},
    {".ttf", AssetKind::kFont},    {".otf", AssetKind::kFont},    {".json", AssetKind::kData},
    {".tmj", AssetKind::kData},    {".tmx", AssetKind::kData},    {".txt", AssetKind::kText},
    {".md", AssetKind::kText},
});

constexpr std::size_t kUtf8TwoBytes = 0x80;
constexpr std::size_t kUtf8ThreeBytes = 0x800;
constexpr std::size_t kUtf8FourBytes = 0x10000;
constexpr unsigned kContinuation = 0x80;
constexpr unsigned kSixBits = 0x3F;
}  // namespace

AssetKind kindOf(std::string_view name, bool isFolder) {
    if (isFolder) {
        return AssetKind::kFolder;
    }
    std::string extension = std::filesystem::path{name}.extension().string();
    std::ranges::transform(extension, extension.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const auto* const found = std::ranges::find(kExtensions, extension, &Extension::first);
    return found == kExtensions.end() ? AssetKind::kOther : found->second;
}

char32_t iconOf(AssetKind kind, bool open) {
    switch (kind) {
        case AssetKind::kFolder:
            return open ? U'' : U'';
        case AssetKind::kScene:
            return U'';
        case AssetKind::kActor:
            return U'';
        case AssetKind::kCapsule:
            return U'';
        case AssetKind::kScript:
            return U'';
        case AssetKind::kImage:
            return U'';
        case AssetKind::kAudio:
            return U'';
        case AssetKind::kFont:
            return U'';
        case AssetKind::kData:
            return U'';
        case AssetKind::kText:
            return U'';
        case AssetKind::kOther:
            break;
    }
    return U'';
}

std::string_view labelOf(AssetKind kind) {
    switch (kind) {
        case AssetKind::kFolder:
            return "Folder";
        case AssetKind::kScene:
            return "Scene";
        case AssetKind::kActor:
            return "Actor";
        case AssetKind::kCapsule:
            return "Capsule";
        case AssetKind::kScript:
            return "Luau script";
        case AssetKind::kImage:
            return "Image";
        case AssetKind::kAudio:
            return "Audio";
        case AssetKind::kFont:
            return "Font";
        case AssetKind::kData:
            return "Data";
        case AssetKind::kText:
            return "Text";
        case AssetKind::kOther:
            break;
    }
    return "File";
}

std::uint32_t colorOf(AssetKind kind) {
    switch (kind) {
        case AssetKind::kFolder:
            return 0xE8B34A;
        case AssetKind::kScene:
            return 0x5AA9F2;
        case AssetKind::kActor:
            return 0x6CCB8B;
        case AssetKind::kCapsule:
            return 0x9C8CF2;
        case AssetKind::kScript:
            return 0x4FC1D9;
        case AssetKind::kImage:
            return 0xE07AB0;
        case AssetKind::kAudio:
            return 0xF08C5A;
        case AssetKind::kFont:
            return 0xC4C9D4;
        case AssetKind::kData:
            return 0xD9C25A;
        case AssetKind::kText:
            return 0xB0B6C2;
        case AssetKind::kOther:
            break;
    }
    return 0x8A90A0;
}

std::string toUtf8(char32_t codePoint) {
    const auto value = static_cast<unsigned>(codePoint);
    std::string out;
    if (value < kUtf8TwoBytes) {
        out.push_back(static_cast<char>(value));
    } else if (value < kUtf8ThreeBytes) {
        out.push_back(static_cast<char>(0xC0U | (value >> 6U)));
        out.push_back(static_cast<char>(kContinuation | (value & kSixBits)));
    } else if (value < kUtf8FourBytes) {
        out.push_back(static_cast<char>(0xE0U | (value >> 12U)));
        out.push_back(static_cast<char>(kContinuation | ((value >> 6U) & kSixBits)));
        out.push_back(static_cast<char>(kContinuation | (value & kSixBits)));
    } else {
        out.push_back(static_cast<char>(0xF0U | (value >> 18U)));
        out.push_back(static_cast<char>(kContinuation | ((value >> 12U) & kSixBits)));
        out.push_back(static_cast<char>(kContinuation | ((value >> 6U) & kSixBits)));
        out.push_back(static_cast<char>(kContinuation | (value & kSixBits)));
    }
    return out;
}

}  // namespace rtype::editor::panels

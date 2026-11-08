/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SdlPlatform
*/

#include "SdlPlatform.hpp"

// The real Vulkan types first: SDL_vulkan.h then uses them instead of declaring its own.
#include <vulkan/vulkan_core.h>
// clang-format off
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
// clang-format on

#ifdef _WIN32
#include <windows.h>

#include <array>
#else
#include <dlfcn.h>
#endif

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::platform::sdl {
namespace {

namespace cp = contract::platform;

/// @return The file the function was loaded from: here, the Vulkan loader the renderer is linked against.
std::string libraryPathOf(PFN_vkGetInstanceProcAddr function) {
#ifdef _WIN32
    HMODULE module = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the OS API takes the address as a string type.
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(function), &module) == 0) {
        return {};
    }
    std::array<char, MAX_PATH> path{};
    const DWORD length = GetModuleFileNameA(module, path.data(), static_cast<DWORD>(path.size()));
    return {path.data(), length};
#else
    Dl_info info{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): dladdr() takes any address as void*.
    if (dladdr(reinterpret_cast<void*>(function), &info) == 0 || info.dli_fname == nullptr) {
        return {};
    }
    return info.dli_fname;
#endif
}

/// @return @p first plus @p offset, for the ranges both enums list in the same order.
cp::input::Key offsetKey(cp::input::Key first, int offset) {
    return static_cast<cp::input::Key>(static_cast<int>(first) + offset);
}

/// @return The engine key at the same physical position, kUnknown for a key the engine does not know.
cp::input::Key toKey(SDL_Scancode scancode) {
    using cp::input::Key;
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return offsetKey(Key::kA, scancode - SDL_SCANCODE_A);
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        return offsetKey(Key::kNum1, scancode - SDL_SCANCODE_1);
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return offsetKey(Key::kF1, scancode - SDL_SCANCODE_F1);
    }
    if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9) {
        return offsetKey(Key::kKeypad1, scancode - SDL_SCANCODE_KP_1);
    }
    switch (scancode) {
        case SDL_SCANCODE_0:
            return Key::kNum0;
        case SDL_SCANCODE_SPACE:
            return Key::kSpace;
        case SDL_SCANCODE_APOSTROPHE:
            return Key::kApostrophe;
        case SDL_SCANCODE_COMMA:
            return Key::kComma;
        case SDL_SCANCODE_MINUS:
            return Key::kMinus;
        case SDL_SCANCODE_PERIOD:
            return Key::kPeriod;
        case SDL_SCANCODE_SLASH:
            return Key::kSlash;
        case SDL_SCANCODE_SEMICOLON:
            return Key::kSemicolon;
        case SDL_SCANCODE_EQUALS:
            return Key::kEqual;
        case SDL_SCANCODE_LEFTBRACKET:
            return Key::kLeftBracket;
        case SDL_SCANCODE_BACKSLASH:
            return Key::kBackslash;
        case SDL_SCANCODE_RIGHTBRACKET:
            return Key::kRightBracket;
        case SDL_SCANCODE_GRAVE:
            return Key::kGrave;
        case SDL_SCANCODE_ESCAPE:
            return Key::kEscape;
        case SDL_SCANCODE_RETURN:
            return Key::kEnter;
        case SDL_SCANCODE_TAB:
            return Key::kTab;
        case SDL_SCANCODE_BACKSPACE:
            return Key::kBackspace;
        case SDL_SCANCODE_INSERT:
            return Key::kInsert;
        case SDL_SCANCODE_DELETE:
            return Key::kDelete;
        case SDL_SCANCODE_RIGHT:
            return Key::kRight;
        case SDL_SCANCODE_LEFT:
            return Key::kLeft;
        case SDL_SCANCODE_DOWN:
            return Key::kDown;
        case SDL_SCANCODE_UP:
            return Key::kUp;
        case SDL_SCANCODE_PAGEUP:
            return Key::kPageUp;
        case SDL_SCANCODE_PAGEDOWN:
            return Key::kPageDown;
        case SDL_SCANCODE_HOME:
            return Key::kHome;
        case SDL_SCANCODE_END:
            return Key::kEnd;
        case SDL_SCANCODE_KP_0:
            return Key::kKeypad0;
        case SDL_SCANCODE_KP_ENTER:
            return Key::kKeypadEnter;
        case SDL_SCANCODE_LSHIFT:
            return Key::kLeftShift;
        case SDL_SCANCODE_LCTRL:
            return Key::kLeftControl;
        case SDL_SCANCODE_LALT:
            return Key::kLeftAlt;
        case SDL_SCANCODE_LGUI:
            return Key::kLeftSuper;
        case SDL_SCANCODE_RSHIFT:
            return Key::kRightShift;
        case SDL_SCANCODE_RCTRL:
            return Key::kRightControl;
        case SDL_SCANCODE_RALT:
            return Key::kRightAlt;
        case SDL_SCANCODE_RGUI:
            return Key::kRightSuper;
        default:
            return Key::kUnknown;
    }
}

cp::input::Mods toMods(SDL_Keymod mods) {
    return {.shift = (mods & SDL_KMOD_SHIFT) != 0,
            .control = (mods & SDL_KMOD_CTRL) != 0,
            .alt = (mods & SDL_KMOD_ALT) != 0,
            .super = (mods & SDL_KMOD_GUI) != 0,
            .capsLock = (mods & SDL_KMOD_CAPS) != 0,
            .numLock = (mods & SDL_KMOD_NUM) != 0};
}

cp::input::MouseButton toMouseButton(std::uint8_t button) {
    using cp::input::MouseButton;
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MouseButton::kLeft;
        case SDL_BUTTON_RIGHT:
            return MouseButton::kRight;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::kMiddle;
        case SDL_BUTTON_X1:
            return MouseButton::kX1;
        case SDL_BUTTON_X2:
            return MouseButton::kX2;
        default:
            return MouseButton::kUnknown;
    }
}

}  // namespace

SdlPlatform::~SdlPlatform() {
    if (_window != nullptr) {
        SDL_DestroyWindow(_window);
    }
    if (_sdlInitialized) {
        SDL_Quit();
    }
}

glm::uvec2 SdlPlatform::getFramebufferSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(_window, &width, &height);
    return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
}

glm::uvec2 SdlPlatform::getWindowSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(_window, &width, &height);
    return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
}

const std::string& SdlPlatform::getTitle() const { return _title; }

void SdlPlatform::setTitle(const std::string& title) {
    _title = title;
    SDL_SetWindowTitle(_window, _title.c_str());
}

void SdlPlatform::setSize(glm::uvec2 size) {
    SDL_SetWindowSize(_window, static_cast<int>(size.x), static_cast<int>(size.y));
}

void SdlPlatform::init(const cp::WindowConfig& config) {
    if (_sdlInitialized) {
        throw std::runtime_error("SdlPlatform::init() called twice");
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    _sdlInitialized = true;
    _title = config.title;

    // Before the window: a SDL_WINDOW_VULKAN window would otherwise load a loader of SDL's choice.
    if (!_loaderPath.empty() && !SDL_Vulkan_LoadLibrary(_loaderPath.c_str())) {
        throw std::runtime_error(std::string("SDL_Vulkan_LoadLibrary failed: ") + SDL_GetError());
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    if (config.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    _window = SDL_CreateWindow(_title.c_str(), static_cast<int>(config.size.x), static_cast<int>(config.size.y), flags);
    if (_window == nullptr) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
    SDL_StartTextInput(_window);
    _isOpen = true;
}

std::vector<cp::Event> SdlPlatform::pollEvents() {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        translate(sdlEvent);
    }
    return std::exchange(_events, {});
}

bool SdlPlatform::isOpen() const noexcept { return _isOpen; }

void SdlPlatform::close() { _isOpen = false; }

void SdlPlatform::setCursorLocked(bool locked) { SDL_SetWindowRelativeMouseMode(_window, locked); }

double SdlPlatform::getTime() const {
    constexpr double kNanosecondsPerSecond = 1e9;
    return static_cast<double>(SDL_GetTicksNS()) / kNanosecondsPerSecond;
}

void SdlPlatform::setClipboardText(std::string_view text) { SDL_SetClipboardText(std::string(text).c_str()); }

std::string SdlPlatform::getClipboardText() const {
    // SDL allocates the text; the unique_ptr gives it back to SDL_free().
    const std::unique_ptr<char, decltype(&SDL_free)> text{SDL_GetClipboardText(), &SDL_free};
    return text != nullptr ? text.get() : "";
}

void SdlPlatform::initLoader(PFN_vkGetInstanceProcAddr loader) {
    if (loader != nullptr) {
        _loaderPath = libraryPathOf(loader);
    }
}

std::vector<const char*> SdlPlatform::getRequiredExtensions() const {
    Uint32 count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (extensions == nullptr) {
        throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions failed: ") + SDL_GetError());
    }
    const std::span view{extensions, count};
    return {view.begin(), view.end()};
}

VkSurfaceKHR SdlPlatform::createSurface(VkInstance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface)) {
        throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
    }
    return surface;
}

// SDL_Event is a C union: reading the member that matches event.type is how SDL is meant to be used.
// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
void SdlPlatform::translate(const SDL_Event& sdlEvent) {
    namespace event = cp::event;
    switch (sdlEvent.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            _isOpen = false;
            _events.emplace_back(event::Closed{});
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            _events.emplace_back(event::Resized{.width = static_cast<std::uint16_t>(sdlEvent.window.data1),
                                                .height = static_cast<std::uint16_t>(sdlEvent.window.data2)});
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            _events.emplace_back(event::FocusChanged{.focused = sdlEvent.type == SDL_EVENT_WINDOW_FOCUS_GAINED});
            break;
        case SDL_EVENT_KEY_DOWN:
            _events.emplace_back(event::KeyPressed{
                .key = toKey(sdlEvent.key.scancode), .mods = toMods(sdlEvent.key.mod), .repeat = sdlEvent.key.repeat});
            break;
        case SDL_EVENT_KEY_UP:
            _events.emplace_back(
                event::KeyReleased{.key = toKey(sdlEvent.key.scancode), .mods = toMods(sdlEvent.key.mod)});
            break;
        case SDL_EVENT_TEXT_INPUT: {
            const char* text = sdlEvent.text.text;
            while (text != nullptr && *text != '\0') {
                const Uint32 codepoint = SDL_StepUTF8(&text, nullptr);
                if (codepoint == 0) {
                    break;
                }
                _events.emplace_back(event::TextEntered{.codepoint = static_cast<char32_t>(codepoint)});
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
            _events.emplace_back(event::MouseMoved{.position = glm::vec2(sdlEvent.motion.x, sdlEvent.motion.y),
                                                   .delta = glm::vec2(sdlEvent.motion.xrel, sdlEvent.motion.yrel)});
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            _events.emplace_back(event::MouseButtonPressed{.button = toMouseButton(sdlEvent.button.button),
                                                           .mods = toMods(SDL_GetModState())});
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            _events.emplace_back(event::MouseButtonReleased{.button = toMouseButton(sdlEvent.button.button),
                                                            .mods = toMods(SDL_GetModState())});
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            _events.emplace_back(event::MouseScrolled{.offset = glm::vec2(sdlEvent.wheel.x, sdlEvent.wheel.y)});
            break;
        default:
            break;
    }
}
// NOLINTEND(cppcoreguidelines-pro-type-union-access)

}  // namespace rtype::platform::sdl

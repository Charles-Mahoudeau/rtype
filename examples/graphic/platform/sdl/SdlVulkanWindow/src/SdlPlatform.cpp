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
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/WindowConfig.hpp"

using namespace rtype::engine;

namespace {

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

/// @return The engine key at the same physical position. Only the common keys: the others read as kUnknown.
input::Key toKey(SDL_Scancode scancode) {
    // Both enums list letters and digits in a row, so ranges map by offset.
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return static_cast<input::Key>(static_cast<int>(input::Key::kA) + (scancode - SDL_SCANCODE_A));
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        return static_cast<input::Key>(static_cast<int>(input::Key::kNum1) + (scancode - SDL_SCANCODE_1));
    }
    switch (scancode) {
        case SDL_SCANCODE_0:
            return input::Key::kNum0;
        case SDL_SCANCODE_SPACE:
            return input::Key::kSpace;
        case SDL_SCANCODE_ESCAPE:
            return input::Key::kEscape;
        case SDL_SCANCODE_RETURN:
            return input::Key::kEnter;
        case SDL_SCANCODE_TAB:
            return input::Key::kTab;
        case SDL_SCANCODE_BACKSPACE:
            return input::Key::kBackspace;
        case SDL_SCANCODE_RIGHT:
            return input::Key::kRight;
        case SDL_SCANCODE_LEFT:
            return input::Key::kLeft;
        case SDL_SCANCODE_DOWN:
            return input::Key::kDown;
        case SDL_SCANCODE_UP:
            return input::Key::kUp;
        default:
            return input::Key::kUnknown;
    }
}

input::Mods toMods(SDL_Keymod mods) {
    return {.shift = (mods & SDL_KMOD_SHIFT) != 0,
            .control = (mods & SDL_KMOD_CTRL) != 0,
            .alt = (mods & SDL_KMOD_ALT) != 0,
            .super = (mods & SDL_KMOD_GUI) != 0,
            .capsLock = (mods & SDL_KMOD_CAPS) != 0,
            .numLock = (mods & SDL_KMOD_NUM) != 0};
}

input::MouseButton toMouseButton(std::uint8_t button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return input::MouseButton::kLeft;
        case SDL_BUTTON_RIGHT:
            return input::MouseButton::kRight;
        case SDL_BUTTON_MIDDLE:
            return input::MouseButton::kMiddle;
        case SDL_BUTTON_X1:
            return input::MouseButton::kX1;
        case SDL_BUTTON_X2:
            return input::MouseButton::kX2;
        default:
            return input::MouseButton::kUnknown;
    }
}

}  // namespace

namespace example {

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

const std::string& SdlPlatform::getTitle() const { return _title; }

void SdlPlatform::setTitle(const std::string& title) {
    _title = title;
    SDL_SetWindowTitle(_window, _title.c_str());
}

void SdlPlatform::setSize(glm::uvec2 size) {
    SDL_SetWindowSize(_window, static_cast<int>(size.x), static_cast<int>(size.y));
}

void SdlPlatform::init(const platform::WindowConfig& config) {
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
    _isOpen = true;
}

std::vector<Event> SdlPlatform::pollEvents() {
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
    const std::span view(extensions, count);
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

}  // namespace example

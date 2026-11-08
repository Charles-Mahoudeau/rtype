/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GlfwPlatform
*/

#include "GlfwPlatform.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "exceptions/WindowExceptions.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"

namespace rtype::platform::glfw {

GlfwPlatform::~GlfwPlatform() {
    if (_window != nullptr) {
        glfwDestroyWindow(_window);
    }
    if (_glfwInitialized) {
        glfwTerminate();
    }
}

glm::uvec2 GlfwPlatform::getWindowSize() const {
    int width = 0;
    int height = 0;
    glfwGetWindowSize(_window, &width, &height);
    return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
}

glm::vec2 GlfwPlatform::getContentScale() const {
    if (_window == nullptr) {
        return {1.0F, 1.0F};
    }
    float x = 1.0F;
    float y = 1.0F;
    glfwGetWindowContentScale(_window, &x, &y);
    return {x, y};
}

glm::uvec2 GlfwPlatform::getFramebufferSize() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
}

const std::string& GlfwPlatform::getTitle() const { return _title; }

void GlfwPlatform::setTitle(const std::string& title) {
    _title = title;
    glfwSetWindowTitle(_window, _title.c_str());
}

void GlfwPlatform::setSize(glm::uvec2 size) {
    glfwSetWindowSize(_window, static_cast<int>(size.x), static_cast<int>(size.y));
}

#ifdef __linux__
namespace {
/// @return The GLFW_PLATFORM hint for a WindowConfig::system: any for "auto".
int platformHint(const std::string& system) {
    if (system == "x11") {
        return GLFW_PLATFORM_X11;
    }
    if (system == "wayland") {
        return GLFW_PLATFORM_WAYLAND;
    }
    return GLFW_ANY_PLATFORM;
}
}  // namespace
#endif

/// @details Window hints:
/// - `GLFW_CLIENT_API = GLFW_NO_API`: by default GLFW creates an OpenGL context with the window. The renderer
///   brings its own graphics API instead, and attaches to the window (Vulkan: through IVulkanSurfaceSource).
/// - `GLFW_RESIZABLE`: from the config. A resizable window makes the renderer rebuild its swapchain on resize.
/// On Linux, `GLFW_PLATFORM` comes from `WindowConfig::system`: any (GLFW takes Wayland in a Wayland session, X11
/// otherwise or if Wayland fails), or the one asked for.
void GlfwPlatform::init(const contract::platform::WindowConfig& config) {
    if (_glfwInitialized) {
        throw exceptions::GLFWWindowException("GlfwPlatform::init() called twice");
    }
#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, platformHint(config.system));
#endif
    if (glfwInit() == GLFW_FALSE) {
        throw exceptions::GLFWWindowException(
            config.system == "auto" ? std::string{"Failed to initialize GLFW"}
                                    : "Failed to initialize GLFW with the window system \"" + config.system + "\"");
    }
    _glfwInitialized = true;
    _title = config.title;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor = nullptr;
    glm::uvec2 size = config.size;
    if (config.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (const GLFWvidmode* mode = glfwGetVideoMode(monitor); mode != nullptr) {
            size = {static_cast<unsigned int>(mode->width), static_cast<unsigned int>(mode->height)};
        }
    }
    _window = glfwCreateWindow(static_cast<int>(size.x), static_cast<int>(size.y), _title.c_str(), monitor, nullptr);
    if (_window == nullptr) {
        throw exceptions::GLFWWindowException("Failed to create GLFW window");
    }
    _isOpen = true;
    registerCallbacks();
}

std::vector<contract::platform::Event> GlfwPlatform::pollEvents() {
    if (_window == nullptr) {
        return {};
    }
    glfwPollEvents();
    pollGamepad();
    _isOpen = glfwWindowShouldClose(_window) == GLFW_FALSE;
    return std::exchange(_events, {});
}

std::vector<contract::platform::Event> GlfwPlatform::waitEvents() {
    if (_window == nullptr) {
        return {};
    }
    glfwWaitEvents();
    pollGamepad();
    _isOpen = glfwWindowShouldClose(_window) == GLFW_FALSE;
    return std::exchange(_events, {});
}

bool GlfwPlatform::isOpen() const noexcept { return _isOpen; }

void GlfwPlatform::close() {
    _isOpen = false;
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
}

void GlfwPlatform::setClipboardText(std::string_view text) {
    if (_window != nullptr) {
        glfwSetClipboardString(_window, std::string{text}.c_str());
    }
}

std::string GlfwPlatform::getClipboardText() const {
    const char* text = _window != nullptr ? glfwGetClipboardString(_window) : nullptr;
    return text != nullptr ? std::string{text} : std::string{};
}

void GlfwPlatform::setCursorLocked(bool locked) {
    glfwSetInputMode(_window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, locked ? GLFW_TRUE : GLFW_FALSE);
    }
    glm::dvec2 position{0.0};
    glfwGetCursorPos(_window, &position.x, &position.y);
    _cursorPosition = position;
}

double GlfwPlatform::getTime() const { return glfwGetTime(); }

void GlfwPlatform::initLoader(PFN_vkGetInstanceProcAddr loader) { glfwInitVulkanLoader(loader); }

std::vector<const char*> GlfwPlatform::getRequiredExtensions() const {
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (extensions == nullptr) {
        throw exceptions::GLFWWindowException("Vulkan is not supported: no required instance extensions");
    }
    const std::span<const char*> view{extensions, count};
    return {view.begin(), view.end()};
}

VkSurfaceKHR GlfwPlatform::createSurface(VkInstance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, _window, nullptr, &surface) != VK_SUCCESS) {
        throw exceptions::GLFWWindowException("Failed to create the Vulkan surface of the window");
    }
    return surface;
}

}  // namespace rtype::platform::glfw

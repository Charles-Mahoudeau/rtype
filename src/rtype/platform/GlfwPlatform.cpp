/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GlfwPlatform
*/

#include "GlfwPlatform.hpp"

// Declares the Vulkan functions of GLFW (glfwInitVulkanLoader, glfwCreateWindowSurface...).
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/platform/WindowConfig.hpp"
#include "exceptions/WindowExceptions.hpp"

namespace rtype::platform {

GlfwPlatform::~GlfwPlatform() {
    if (_window != nullptr) {
        glfwDestroyWindow(_window);
    }
    if (_glfwInitialized) {
        glfwTerminate();
    }
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

/// @details Window hints:
/// - `GLFW_CLIENT_API = GLFW_NO_API`: by default GLFW creates an OpenGL context with the window. The renderer
///   brings its own graphics API instead, and attaches to the window (Vulkan: through IVulkanSurfaceSource).
/// - `GLFW_RESIZABLE`: from the config. A resizable window makes the renderer rebuild its swapchain on resize.
void GlfwPlatform::init(const engine::platform::WindowConfig& config) {
    if (_glfwInitialized) {
        throw exceptions::GLFWWindowException("GlfwPlatform::init() called twice");
    }
    if (glfwInit() == GLFW_FALSE) {
        throw exceptions::GLFWWindowException("Failed to initialize GLFW");
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

std::vector<engine::Event> GlfwPlatform::pollEvents() {
    if (_window == nullptr) {
        return {};
    }
    _events.clear();
    glfwPollEvents();
    pollGamepad();
    _isOpen = glfwWindowShouldClose(_window) == GLFW_FALSE;
    return std::exchange(_events, {});
}

bool GlfwPlatform::isOpen() const noexcept { return _isOpen; }

void GlfwPlatform::close() {
    _isOpen = false;
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
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
    const std::span<const char*> view(extensions, count);
    return {view.begin(), view.end()};
}

VkSurfaceKHR GlfwPlatform::createSurface(VkInstance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, _window, nullptr, &surface) != VK_SUCCESS) {
        throw exceptions::GLFWWindowException("Failed to create the Vulkan surface of the window");
    }
    return surface;
}

}  // namespace rtype::platform

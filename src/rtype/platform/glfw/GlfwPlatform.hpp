/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GlfwPlatform
*/

#pragma once

#include <array>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/IPlatform.hpp"
#include "engine/platform/WindowConfig.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"

struct GLFWwindow;

namespace rtype::platform::glfw {

/// @brief The GLFW implementation of IPlatform: the application window, and the source of every engine event.
///
/// @details GLFW stays an implementation detail: this header does not include it, and every callback is
/// translated into an engine::Event returned by pollEvents(). The window has no graphics context
/// (GLFW_NO_API): a Vulkan renderer attaches to it through IVulkanSurfaceSource.
///
/// @warning On macOS 11.3+, controllers natively handled by Apple's GameController
/// framework (e.g. Switch Pro Controller) are detected by GLFW but never send updates,
/// so they read as idle.
class GlfwPlatform final : public engine::platform::IPlatform, public interop::vulkan::IVulkanSurfaceSource {
  public:
    /// @brief Does not touch GLFW yet: the renderer may call initLoader() first, then the engine calls init().
    GlfwPlatform() = default;
    /// @brief Destroys the window and terminates GLFW, if init() succeeded.
    ~GlfwPlatform() override;
    GlfwPlatform(const GlfwPlatform&) = delete;
    GlfwPlatform& operator=(const GlfwPlatform&) = delete;
    GlfwPlatform(GlfwPlatform&&) = delete;
    GlfwPlatform& operator=(GlfwPlatform&&) = delete;

    [[nodiscard]] glm::uvec2 getFramebufferSize() const override;
    [[nodiscard]] const std::string& getTitle() const override;
    void setTitle(const std::string& title) override;
    void setSize(glm::uvec2 size) override;

    /// @throws GLFWWindowException If GLFW or the window cannot be initialized, or init() was already called.
    void init(const engine::platform::WindowConfig& config) override;

    [[nodiscard]] std::vector<engine::Event> pollEvents() override;
    [[nodiscard]] bool isOpen() const noexcept override;
    void close() override;
    void setCursorLocked(bool locked) override;
    [[nodiscard]] double getTime() const override;

    /// @name IVulkanSurfaceSource
    /// @{

    /// @brief glfwInitVulkanLoader(): GLFW then uses the renderer's Vulkan loader instead of dlopen()-ing its own.
    void initLoader(PFN_vkGetInstanceProcAddr loader) override;

    /// @throws GLFWWindowException If no Vulkan driver exposes the surface extensions.
    [[nodiscard]] std::vector<const char*> getRequiredExtensions() const override;

    /// @brief glfwCreateWindowSurface(): the Vulkan surface of the window.
    /// @throws GLFWWindowException If the surface cannot be created.
    [[nodiscard]] VkSurfaceKHR createSurface(VkInstance instance) override;
    /// @}

  private:
    static constexpr std::size_t kGamepadButtonCount = static_cast<std::size_t>(engine::input::GamepadButton::kCount);
    static constexpr std::size_t kGamepadAxisCount = static_cast<std::size_t>(engine::input::GamepadAxis::kCount);

    /// @brief Binds this instance to the GLFW handle and installs every event callback.
    void registerCallbacks();

    /// @return The GlfwPlatform bound to a GLFW handle by registerCallbacks().
    static GlfwPlatform& fromHandle(GLFWwindow* handle);

    /// @name GLFW callbacks
    /// @brief Translate a GLFW callback into an engine event pushed to _events.
    /// @{
    static void onClose(GLFWwindow* handle);
    static void onFramebufferResize(GLFWwindow* handle, int width, int height);
    static void onFocus(GLFWwindow* handle, int focused);
    static void onKey(GLFWwindow* handle, int key, int scancode, int action, int mods);
    static void onChar(GLFWwindow* handle, unsigned int codepoint);
    static void onCursorPos(GLFWwindow* handle, double x, double y);
    static void onMouseButton(GLFWwindow* handle, int button, int action, int mods);
    static void onScroll(GLFWwindow* handle, double xoffset, double yoffset);
    /// @}

    /// @brief GLFW has no gamepad callbacks: compares the gamepad with the previous frame and emits the changes.
    void pollGamepad();
    /// @brief Releases every button and axis, then emits GamepadDisconnected.
    void disconnectGamepad();

    bool _isOpen{false};                 ///< True between init() and the window closing.
    bool _glfwInitialized{false};        ///< True once glfwInit() succeeded, used to call glfwTerminate().
    GLFWwindow* _window{nullptr};        ///< The GLFW window instance, created by init().
    std::string _title;                  ///< Window title.
    std::vector<engine::Event> _events;  ///< Events received since the last poll.
    glm::vec2 _cursorPosition{0.0F};     ///< Last cursor position, to compute MouseMoved::delta.
    bool _hasCursorPosition{false};      ///< False until the first cursor event, to avoid a first jump.
    int _gamepadId{-1};                  ///< GLFW joystick id of the reported gamepad, -1 if none.
    std::array<bool, kGamepadButtonCount> _gamepadButtons{};  ///< Button state sent in the last events.
    std::array<float, kGamepadAxisCount> _gamepadAxes{};      ///< Axis values sent in the last events.
};
}  // namespace rtype::platform::glfw

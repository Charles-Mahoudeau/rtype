/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SdlPlatform
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/platform/IPlatform.hpp"
#include "engine/platform/WindowConfig.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"

struct SDL_Window;
union SDL_Event;

namespace example {

/// @brief An IPlatform written with SDL3, to show how to plug another windowing library into the engine.
///
/// @details Same role as rtype::platform::glfw::GlfwPlatform: a window, and every native event translated into an
/// engine::Event. It also implements IVulkanSurfaceSource, so the engine's VulkanRenderer can use it, through
/// SDL_Vulkan_*:
/// - initLoader(): SDL_Vulkan_LoadLibrary() on the file of the renderer's loader, so SDL and the renderer share it;
/// - getRequiredExtensions(): SDL_Vulkan_GetInstanceExtensions();
/// - createSurface(): SDL_Vulkan_CreateSurface().
///
/// Kept short on purpose: only the common keys and the mouse are translated, and gamepads are ignored.
class SdlPlatform final : public rtype::engine::platform::IPlatform,
                          public rtype::interop::vulkan::IVulkanSurfaceSource {
  public:
    /// @brief Does not touch SDL yet: the renderer may call initLoader() first, then the engine calls init().
    SdlPlatform() = default;
    /// @brief Destroys the window and shuts SDL down, if init() succeeded.
    ~SdlPlatform() override;
    SdlPlatform(const SdlPlatform&) = delete;
    SdlPlatform& operator=(const SdlPlatform&) = delete;
    SdlPlatform(SdlPlatform&&) = delete;
    SdlPlatform& operator=(SdlPlatform&&) = delete;

    [[nodiscard]] glm::uvec2 getFramebufferSize() const override;
    [[nodiscard]] const std::string& getTitle() const override;
    void setTitle(const std::string& title) override;
    void setSize(glm::uvec2 size) override;

    /// @throws std::runtime_error If SDL, the Vulkan loader or the window cannot be initialized.
    void init(const rtype::engine::platform::WindowConfig& config) override;

    [[nodiscard]] std::vector<rtype::engine::Event> pollEvents() override;
    [[nodiscard]] bool isOpen() const noexcept override;
    void close() override;
    void setCursorLocked(bool locked) override;
    [[nodiscard]] double getTime() const override;

    /// @brief Remembers the file of the renderer's loader; init() loads it with SDL_Vulkan_LoadLibrary().
    /// @details SDL only loads a loader from a path, not from a function pointer: the path is found from the
    /// address of the entry point (dladdr(), or GetModuleFileName() on Windows).
    void initLoader(PFN_vkGetInstanceProcAddr loader) override;

    /// @throws std::runtime_error If SDL has no Vulkan support.
    [[nodiscard]] std::vector<const char*> getRequiredExtensions() const override;

    /// @throws std::runtime_error If the surface cannot be created.
    [[nodiscard]] VkSurfaceKHR createSurface(VkInstance instance) override;

  private:
    /// @brief Appends the engine event matching an SDL event to _events, if there is one.
    void translate(const SDL_Event& sdlEvent);

    bool _sdlInitialized{false};                ///< True once SDL_Init() succeeded, used to call SDL_Quit().
    bool _isOpen{false};                        ///< True between init() and the window closing.
    SDL_Window* _window{nullptr};               ///< The SDL window, created by init().
    std::string _title;                         ///< Window title.
    std::string _loaderPath;                    ///< File of the renderer's Vulkan loader; empty: SDL picks one.
    std::vector<rtype::engine::Event> _events;  ///< Events translated since the last poll.
};
}  // namespace example

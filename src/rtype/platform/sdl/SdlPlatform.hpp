/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SdlPlatform
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "interop/vulkan/IVulkanSurfaceSource.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/platform/sdl/Export.hpp"

struct SDL_Window;
union SDL_Event;

namespace rtype::platform::sdl {

/// @brief The window and the input devices, through SDL3: the same role as the GLFW platform, registered as "sdl".
///
/// @details Every native event is translated into a contract::platform::Event. It implements IVulkanSurfaceSource, so
/// the Vulkan renderer draws in its window through SDL_Vulkan_*:
/// - initLoader(): SDL_Vulkan_LoadLibrary() on the file of the renderer's loader, so SDL and the renderer share it;
/// - getRequiredExtensions(): SDL_Vulkan_GetInstanceExtensions();
/// - createSurface(): SDL_Vulkan_CreateSurface().
///
/// Gamepads are not reported yet.
class RTYPE_PLATFORM_SDL_API SdlPlatform final : public contract::platform::IPlatform,
                                                 public interop::vulkan::IVulkanSurfaceSource {
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
    [[nodiscard]] glm::uvec2 getWindowSize() const override;
    [[nodiscard]] const std::string& getTitle() const override;
    void setTitle(const std::string& title) override;
    void setSize(glm::uvec2 size) override;

    /// @throws std::runtime_error If SDL, the Vulkan loader or the window cannot be initialized.
    void init(const contract::platform::WindowConfig& config) override;

    [[nodiscard]] std::vector<contract::platform::Event> pollEvents() override;
    [[nodiscard]] bool isOpen() const noexcept override;
    void close() override;
    void setCursorLocked(bool locked) override;
    [[nodiscard]] double getTime() const override;
    void setClipboardText(std::string_view text) override;
    [[nodiscard]] std::string getClipboardText() const override;

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

    bool _sdlInitialized{false};                     ///< True once SDL_Init() succeeded, used to call SDL_Quit().
    bool _isOpen{false};                             ///< True between init() and the window closing.
    SDL_Window* _window{nullptr};                    ///< The SDL window, created by init().
    std::string _title;                              ///< Window title.
    std::string _loaderPath;                         ///< File of the renderer's Vulkan loader; empty: SDL picks one.
    std::vector<contract::platform::Event> _events;  ///< Events translated since the last poll.
};

}  // namespace rtype::platform::sdl

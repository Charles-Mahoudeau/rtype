/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Export.hpp"
#include "core/DebugMessenger.hpp"
#include "core/Instance.hpp"
#include "engine/graphics/Camera.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/graphics/RectShape.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/platform/IPlatform.hpp"

namespace rtype::vulkan {

/// @brief The Vulkan implementation of IRenderer.
///
/// @details Skeleton for now: init() creates the instance, the debug messenger (if requested) and the surface of
/// the window. Every other function throws UnsupportedFeatureException until it is implemented.
///
/// Needs a platform that overrides getRequiredExtensions() and createSurface() (GlfwPlatform does).
class RTYPE_VULKAN_API VulkanRenderer final : public engine::graphics::IRenderer {
  public:
    /// @brief Settings of the Vulkan backend, given at construction. Set only what differs from the defaults:
    /// @code
    /// VulkanRenderer renderer(VulkanRenderer::Config{.layers = {"VK_LAYER_KHRONOS_validation"}, .debugging =
    /// true});
    /// @endcode
    struct Config {
        std::string engineName = "R-Type Engine";       ///< Reported to the driver, with the window title as app name.
        std::uint32_t apiVersion = VK_API_VERSION_1_3;  ///< Highest Vulkan version the renderer uses.
        std::vector<const char*> layers;
        std::vector<const char*> extraExtensions;  ///< Enabled on top of the ones the window needs.
        bool debugging = false;                    ///< Whether to create the DebugMessenger
        vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;  ///< Minimum severity of messages the DebugMessenger
                                                                 ///< prints.
    };

    explicit VulkanRenderer(Config config);
    /// @brief A renderer with the default Config.
    VulkanRenderer();
    ~VulkanRenderer() override = default;
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    /// @return vkGetInstanceProcAddr of the loader this library is linked against.
    [[nodiscard]] engine::platform::ProcAddress getLoaderEntryPoint() const override;

    /// @brief Creates the instance (with the platform's extensions), the debug messenger and the window surface.
    /// @throws exceptions::UnsupportedFeatureException If the platform cannot create a Vulkan surface.
    /// @throws std::runtime_error If a layer or an extension is missing, or init() was already called.
    void init(engine::platform::IPlatform& platform) override;

    void resize(glm::uvec2 framebufferSize) override;

    /// @name Not implemented yet: throw UnsupportedFeatureException
    /// @{
    [[nodiscard]] engine::graphics::TextureId createTexture(const engine::graphics::TextureDesc& desc,
                                                            std::span<const std::byte> pixels) override;
    void destroy(engine::graphics::TextureId texture) override;
    void beginFrame(const engine::graphics::Color& clearColor) override;
    void endFrame() override;
    void setCamera(const engine::graphics::Camera& camera) override;
    void draw(const engine::graphics::Sprite& sprite) override;
    void draw(const engine::graphics::RectShape& rect) override;
    /// @}

  private:
    /// @throws exceptions::UnsupportedFeatureException Always, naming the function.
    [[noreturn]] static void notImplemented(std::string_view function);

    Config _config;                                       ///< Settings given at construction.
    glm::uvec2 _framebufferSize{0};                       ///< Size of the window's framebuffer, in pixels.
    std::optional<core::Instance> _instance;              ///< Created by init().
    std::optional<core::DebugMessenger> _debugMessenger;  ///< Created by init() when validation is enabled.
    vk::raii::SurfaceKHR _surface = nullptr;              ///< The window's surface, created by init().
};
}  // namespace rtype::vulkan

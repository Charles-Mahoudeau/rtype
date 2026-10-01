/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <string_view>
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
/// @details Skeleton for now: init() creates the instance, the debug messenger (with validation) and the surface of
/// the window. Every other function throws UnsupportedFeatureException until it is implemented.
///
/// Needs a platform that overrides getRequiredExtensions() and createSurface() (GlfwPlatform does).
class RTYPE_VULKAN_API VulkanRenderer final : public engine::graphics::IRenderer {
  public:
    /// @param enableValidation Enables VK_LAYER_KHRONOS_validation and prints its messages: debug builds only.
    explicit VulkanRenderer(bool enableValidation = false);
    ~VulkanRenderer() override = default;
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    /// @return vkGetInstanceProcAddr of the loader this library is linked against.
    [[nodiscard]] engine::platform::ProcAddress getLoaderEntryPoint() const override;

    /// @brief Creates the instance (with the platform's extensions), the debug messenger and the window surface.
    /// @throws exceptions::UnsupportedFeatureException If the platform cannot create a Vulkan surface.
    /// @throws std::runtime_error If an extension or the validation layer is missing, or init() was already called.
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

    bool _enableValidation;                               ///< Validation layer and debug messenger requested.
    glm::uvec2 _framebufferSize{0};                       ///< Size of the window's framebuffer, in pixels.
    std::optional<core::Instance> _instance;              ///< Created by init().
    std::optional<core::DebugMessenger> _debugMessenger;  ///< Created by init() when validation is enabled.
    vk::raii::SurfaceKHR _surface = nullptr;              ///< The window's surface, created by init().
};
}  // namespace rtype::vulkan

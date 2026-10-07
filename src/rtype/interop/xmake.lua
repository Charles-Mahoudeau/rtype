-- Interop: one header-only target per graphics API, with the interfaces a platform implements to serve the
-- renderers of that API. The platform and the renderer both depend on it; neither depends on the other.

-- Vulkan: IVulkanSurfaceSource.
target("interop-vulkan")
    set_kind("headeronly")
    add_headerfiles("Export.hpp", "vulkan/**.hpp")
    -- Headers are included as "interop/vulkan/...".
    add_includedirs("..", {public = true})
    add_packages("vulkan-headers", {public = true})

-- GLFW platform: the window and the input devices, through GLFW. The only module that includes GLFW.
target("platform-glfw")
    add_rules("rtype.plugin_library", {folder = "plugins/backends"})
    set_kind("shared")
    set_basename("rtype-platform-glfw")
    add_defines("RTYPE_PLATFORM_GLFW_BUILD", {public = false})
    -- Public: Registration.hpp includes PlatformRegistry.
    add_deps("core", "contract-platform", {public = true})
    add_packages("glfw")
    -- GlfwPlatform implements IVulkanSurfaceSource. Public: GlfwPlatform.hpp includes it.
    add_deps("interop-vulkan", {public = true})
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "platform/glfw/...".
    add_includedirs("../..", {public = true})

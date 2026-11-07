-- GLFW platform: the window and the input devices, through GLFW. The only module that includes GLFW.
target("platform-glfw")
    add_rules("rtype.plugin_library", {folder = "plugins/backends"})
    set_kind("shared")
    set_basename("rtype-platform-glfw")
    add_defines("RTYPE_PLATFORM_GLFW_BUILD", {public = false})
    -- Public: Registration.hpp includes PlatformRegistry.
    add_deps("core", "contract-platform", {public = true})
    add_packages("glfw")
    -- On Linux, GLFW opens the X11 and Wayland libraries when it starts. The package lists them (and the thirty xcb
    -- libraries they need) as links all the same, which would make the library need them to load, and a machine
    -- without a display server (the headless server) would not load it. Only GLFW itself is linked.
    before_link(function (target)
        if target:is_plat("linux") then
            target:pkg("glfw"):set("links", {"glfw3"})
        end
    end)
    -- GlfwPlatform implements IVulkanSurfaceSource. Public: GlfwPlatform.hpp includes it.
    add_deps("interop-vulkan", {public = true})
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "platform/glfw/...".
    add_includedirs("../..", {public = true})

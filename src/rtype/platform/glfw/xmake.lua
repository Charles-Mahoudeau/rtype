-- GLFW platform: the window and the input devices, through GLFW. The only module that includes GLFW.
target("platform-glfw")
    set_kind("static")
    set_basename("rtype-platform-glfw")
    add_deps("engine-core")
    add_packages("glfw")
    -- GlfwPlatform implements IVulkanSurfaceSource. Public: GlfwPlatform.hpp includes it.
    add_deps("interop-vulkan", {public = true})
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "platform/glfw/...".
    add_includedirs("../..", {public = true})
    -- Linked into the shared modules.
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})

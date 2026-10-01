-- Platform layer: window and devices. The only module that includes GLFW.
target("platform")
    set_kind("static")
    set_basename("rtype-platform")
    add_deps("engine-core")
    add_packages("glfw")
    -- Headers only, private: GlfwPlatform.cpp calls the Vulkan functions of GLFW (loader, surface).
    add_packages("vulkan-headers")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "platform/...".
    add_includedirs("..", {public = true})
    -- Linked into the shared modules.
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})

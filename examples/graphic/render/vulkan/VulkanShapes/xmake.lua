target("VulkanShapes")
    set_kind("binary")
    set_default(false)
    add_deps("plugin-input", "contract-renderer", "plugin-graphics", "platform-glfw", "render-vulkan")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

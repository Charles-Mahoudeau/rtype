target("VulkanInstance")
    set_kind("binary")
    set_default(false)
    add_deps("engine-core", "platform", "vulkan")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

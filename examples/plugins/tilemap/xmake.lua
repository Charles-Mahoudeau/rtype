target("tilemap")
    set_kind("binary")
    set_default(false)
    add_deps("plugins", "platform-glfw", "render-vulkan")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

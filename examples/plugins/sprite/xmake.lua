target("sprite")
    set_kind("binary")
    set_default(false)
    add_deps("plugin-shader", "plugin-input", "contract-renderer", "plugin-graphics", "plugin-sprite", "plugin-transform", "platform-glfw", "render-vulkan")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

target("particle")
    set_kind("binary")
    set_default(false)
    add_deps("plugin-shader", "contract-platform", "contract-renderer", "plugin-graphics", "plugin-input", "plugin-particle", "plugin-sprite", "plugin-text", "plugin-transform", "platform-glfw", "render-vulkan")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

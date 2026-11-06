target("rtype_duel")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "plugin-particle", "plugin-shader", "core", "contract-audiodevice", "contract-platform", "contract-renderer", "plugin-audio", "plugin-graphics", "plugin-input", "plugin-physics", "plugin-physics-debug", "plugin-shape", "plugin-sprite", "plugin-text", "plugin-tilemap", "plugin-transform", "replication", "platform-glfw", "render-vulkan", "audio-miniaudio")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

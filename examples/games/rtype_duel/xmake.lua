target("rtype_duel")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "plugin-particle", "plugin-shader", "core", "contract-audiodevice", "contract-platform", "contract-renderer", "plugin-audio", "plugin-graphics", "plugin-input", "plugin-physics", "plugin-physics-debug", "plugin-sprite", "plugin-text", "plugin-tilemap", "plugin-transform", "replication", "platform-glfw", "render-vulkan", "audio-miniaudio")

    add_files("src/**.cpp")
    add_includedirs("src")

    set_rundir("$(projectdir)")

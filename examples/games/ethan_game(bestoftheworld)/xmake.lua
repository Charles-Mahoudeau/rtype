target("ethan_game(bestoftheworld)")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "plugin-particle", "plugin-tilemap", "plugin-text", "plugin-shader", "plugin-input", "core", "contract-audiodevice", "contract-renderer", "plugin-audio", "plugin-graphics", "plugin-sprite", "plugin-transform", "platform-glfw", "render-vulkan", "audio-miniaudio")

    add_files("src/**.cpp")
    add_includedirs("src")

    set_rundir("$(projectdir)")

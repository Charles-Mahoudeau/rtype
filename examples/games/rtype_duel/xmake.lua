target("rtype_duel")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "core", "plugins", "replication", "platform-glfw", "render-vulkan", "audio-miniaudio")

    add_files("src/**.cpp")
    add_includedirs("src")

    set_rundir("$(projectdir)")

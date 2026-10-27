-- The executable: wires the plugins with the platform, the renderer and the scripting.
target("engine")
    set_kind("binary")
    add_deps("plugins", "platform-glfw", "luau", "render-vulkan", "replication", "audio-miniaudio")
    add_files("main.cpp")
    set_rundir("$(projectdir)")

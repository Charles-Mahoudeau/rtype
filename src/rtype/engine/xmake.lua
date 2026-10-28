-- The executable: wires the plugins with the platform, the renderer and the scripting.
target("engine")
    set_kind("binary")
    add_deps("core", "plugins", "platform-glfw", "luau", "render-vulkan", "replication", "audio-miniaudio")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_includedirs("$(projectdir)/src")
    set_rundir("$(projectdir)")

-- r-type_client: the generic client. It registers the backends (GLFW, Vulkan, miniaudio) and lets the Engine run the
-- game directory; the binary is copied to the repository root after the build, as the subject asks.
target("r-type_client")
    set_kind("binary")
    add_deps("engine", "platform-glfw", "render-vulkan", "audio-miniaudio")
    add_files("**.cpp")
    add_rules("rtype.copy_to_root")

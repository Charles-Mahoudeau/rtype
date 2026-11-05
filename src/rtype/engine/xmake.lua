-- The engine: loads the project settings (Luau), fills the backend and audio registries, and builds the application
-- with the plugins the project lists (or the default ones).
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    -- Public: the engine's headers use the App, the settings and the plugins' registries.
    add_deps("ecs", "core", "plugins", {public = true})
    add_deps("luau")
    -- Private: only the engine's .cpp files register the backends, by name, in the registries.
    add_deps("platform-glfw", "render-vulkan", "audio-miniaudio")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_defines("RTYPE_ENGINE_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})

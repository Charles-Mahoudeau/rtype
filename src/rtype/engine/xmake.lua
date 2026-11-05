-- The engine: reads a game's project settings (Luau), builds its App with the engine's plugins (backend, audio,
-- network, scripting) and runs it. It names no backend: the executables register them.
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    -- Public: Engine.hpp and EngineConfig.hpp expose the App, the settings and the backend and audio registries.
    add_deps("core", "ecs", "plugins", {public = true})
    add_deps("luau", "scripting", "replication")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_defines("RTYPE_ENGINE_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})

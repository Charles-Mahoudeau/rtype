-- The engine: reads a game's project settings (Luau), builds its App (network, scripting, and the plugins the program
-- adds) and runs it. It names no backend: the executables add the plugins they use.
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    -- Public: Engine.hpp and EngineConfig.hpp expose the App and the settings.
    add_deps("core", "ecs", {public = true})
    -- The transform plugin is the only one the Engine names: every game has it.
    add_deps("luau", "scripting", "replication", "plugin-transform")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_defines("RTYPE_ENGINE_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})

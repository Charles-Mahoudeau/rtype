-- The engine: reads a game's project settings (Luau), builds its App (network, scripting, and the plugins the program
-- adds) and runs it. It names no backend: the executables add the plugins they use.
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    -- Public: Engine.hpp and EngineConfig.hpp expose the App and the settings.
    add_deps("core", "ecs", {public = true})
    -- The plugins the Engine names: the transform plugin, which every game has, the generic ones project.luau lists
    -- (GamePlugins), and the input plugin, which NetworkInput reads.
    add_deps("luau", "scripting", "replication", "plugin-transform", "plugin-input", "plugin-physics", "plugin-physics-debug")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_defines("RTYPE_ENGINE_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})

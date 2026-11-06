-- The public root key game plugins are verified against in a release build, in hexadecimal. The secret key stays out of
-- the repository. Without it, a release build refuses every game plugin.
option("plugin_root_key", {default = "", showmenu = true, description = "Public root key of the game plugins (hex)"})
add_requires("libsodium", {alias = "libsodium"})

-- The engine: reads a game's project settings (Luau), builds its App (network, scripting, and the plugins the program
-- adds) and runs it. It names no backend: the executables add the plugins they use.
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    -- Public: Engine.hpp and EngineConfig.hpp expose the App and the settings.
    add_deps("core", "ecs", {public = true})
    -- The plugins the Engine names: the transform plugin, which every game has, and the input plugin, which NetworkInput
    -- reads. The others are loaded from their libraries.
    add_deps("luau", "scripting", "replication", "plugin-transform", "plugin-input")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_defines("RTYPE_ENGINE_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})
    -- Verifies the game plugins' signatures. Private: only plugin/PluginSignature.cpp includes it.
    add_packages("libsodium")
    add_defines('RTYPE_PLUGIN_ROOT_KEY="' .. (get_config("plugin_root_key") or "") .. '"', {public = false})
    -- dlopen() is in libdl on older Linux systems.
    if is_plat("linux") then
        add_syslinks("dl")
    end

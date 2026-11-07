-- The public root key game plugins are verified against in a release build, in hexadecimal. The secret key stays out of
-- the repository. Without it, a release build refuses every game plugin.
option("plugin_root_key", {default = "", showmenu = true, description = "Public root key of the game plugins (hex)"})
-- Built by xmake and linked statically: the system's is a shared library a release does not hold
-- (docs/release/self-contained.md).
add_requires("libsodium", {alias = "libsodium", system = false})

-- The engine: reads a game's project settings (Luau), builds its App (network, scripting, and the plugins the program
-- adds) and runs it. It names no backend: the executables add the plugins they use.
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    -- Public: Engine.hpp and EngineConfig.hpp expose the App and the settings.
    add_deps("core", "ecs", {public = true})
    -- No plugin: the Engine loads them all, the transform and input plugins too, from their libraries.
    add_deps("luau", "scripting", "replication")
    -- Those two plugin libraries are in `plugins/`, next to `lib/`, which holds the engine.
    if not is_plat("windows") then
        add_rpathdirs("@loader_path/../plugins")
    end
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

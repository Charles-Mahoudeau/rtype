-- The Luau bridge: turns what scripts declare into ECS data.
target("scripting")
    -- Static, linked into the executable: no plugin links it (rtype.static_modules enforces it).
    set_kind("static")
    set_basename("rtype-scripting")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_deps("core", "ecs", "luau", {public = true})
    add_defines("RTYPE_SCRIPTING_BUILD", {public = false})
    add_defines("RTYPE_SCRIPTING_STATIC", {public = true})
    add_includedirs("$(projectdir)/src", {public = true})
    add_packages("tl_expected", {public = true})
    -- Embeds the prelude of the setup forms as a byte array, included as "FormsPrelude.luau.h".
    add_rules("utils.bin2c", {extensions = {".luau"}})
    add_files("setup/FormsPrelude.luau")
    -- Same for the helpers of the debug console.
    add_files("console/ConsolePrelude.luau")

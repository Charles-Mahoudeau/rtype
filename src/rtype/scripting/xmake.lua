-- The Luau bridge: turns what scripts declare into ECS data.
target("scripting")
    set_kind("shared")
    set_basename("rtype-scripting")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_deps("core", "ecs", "luau", {public = true})
    add_defines("RTYPE_SCRIPTING_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})
    add_packages("tl_expected", {public = true})
    -- Embeds the prelude of the setup forms as a byte array, included as "FormsPrelude.luau.h".
    add_rules("utils.bin2c", {extensions = {".luau"}})
    add_files("setup/FormsPrelude.luau")

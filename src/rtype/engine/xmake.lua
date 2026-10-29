-- The engine: loads the project settings (Luau) and, later, owns the application.
target("engine")
    set_kind("shared")
    set_basename("rtype-engine")
    add_deps("ecs", "luau")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_defines("RTYPE_ENGINE_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})

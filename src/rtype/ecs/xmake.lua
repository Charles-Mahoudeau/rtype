target("ecs")
    set_kind("shared")
    set_basename("rtype-ecs")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_ECS_BUILD", {public = false})
    -- Headers are included as "rtype/ecs/...".
    add_includedirs("$(projectdir)/src", {public = true})

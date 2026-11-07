target("replication")
    -- Static, linked into the executable: no plugin links it (rtype.static_modules enforces it).
    set_kind("static")
    set_basename("rtype-replication")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_REPLICATION_BUILD", {public = false})
    add_defines("RTYPE_REPLICATION_STATIC", {public = true})
    -- Public: the headers of replication use the types of both libraries.
    add_deps("ecs", "network", {public = true})

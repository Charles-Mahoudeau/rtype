target("replication")
    set_kind("shared")
    set_basename("rtype-replication")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_REPLICATION_BUILD", {public = false})
    -- Public: the headers of replication use the types of both libraries.
    add_deps("ecs", "network", {public = true})

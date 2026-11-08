target("replication")
    -- Static, linked into the executable: no plugin links it (rtype.static_modules enforces it).
    set_kind("static")
    set_basename("rtype-replication")
    add_files("**.cpp")
    -- Embeds the Luau forms of the packets (packet/NetForms.luau) as a byte array, included as "NetForms.luau.h".
    add_rules("utils.bin2c", {extensions = {".luau"}})
    add_files("packet/NetForms.luau")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_REPLICATION_BUILD", {public = false})
    add_defines("RTYPE_REPLICATION_STATIC", {public = true})
    -- Public: the headers of replication use the types of both libraries.
    add_deps("ecs", "network", {public = true})

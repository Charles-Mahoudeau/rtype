-- r-type_client: the generic client. It lets the Engine run the game directory, which loads the plugins and backends
-- from their libraries; the binary is copied to the repository root after the build, as the subject asks.
target("r-type_client")
    set_kind("binary")
    add_deps("engine")
    -- Built first, not linked: the Engine loads them. The server too, so that the two never copy the DLLs they share to
    -- the root at once, which Windows refuses (rtype.copy_to_root).
    add_deps("plugin-libraries", "backend-libraries", "r-type_server", {inherit = false})
    add_files("**.cpp")
    add_rules("rtype.copy_to_root")

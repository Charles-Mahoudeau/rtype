-- r-type_server: the generic headless server. It lets the Engine run the game directory; the binary is copied to the
-- repository root after the build, as the subject asks.
target("r-type_server")
    set_kind("binary")
    add_deps("engine")
    -- Built first, not linked: the Engine loads them.
    add_deps("plugin-libraries", {inherit = false})
    add_files("**.cpp")
    add_rules("rtype.copy_to_root")

-- aylo-runtime: the generic program that runs a game, as a client or, with `--server`, as the headless server. It lets
-- the Engine run the game directory, which loads the plugins and backends from their libraries; the binary is copied to
-- the repository root after the build.
target("aylo-runtime")
    set_kind("binary")
    add_deps("engine")
    -- Built first, not linked: the Engine loads them.
    add_deps("plugin-libraries", "backend-libraries", {inherit = false})
    add_files("**.cpp")
    add_rules("rtype.copy_to_root")

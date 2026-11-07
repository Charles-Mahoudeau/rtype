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
    -- What `--version` prints: the version (with the nightly stamp) and the commit.
    on_load(function (target)
        import("engine_version")
        target:add("defines", 'RTYPE_ENGINE_VERSION="' .. engine_version.full() .. '"')
        target:add("defines", 'RTYPE_ENGINE_COMMIT="' .. engine_version.commit() .. '"')
    end)

-- aylo-sign: makes the keys and signatures of the game plugins (docs/Plugins.md).
target("aylo-sign")
    set_kind("binary")
    set_default(false)
    add_deps("engine")
    add_packages("libsodium")
    add_files("main.cpp")
    -- In the release zip it is in `tools/`, next to `runtime/`, which holds the libraries it links (`lib/`, and the
    -- plugins the engine links in `plugins/`). On Windows the packaging copies the DLLs next to it, as there is no
    -- rpath there.
    if not is_plat("windows") then
        add_rpathdirs("@loader_path/../runtime/lib", "@loader_path/../runtime/plugins")
    end

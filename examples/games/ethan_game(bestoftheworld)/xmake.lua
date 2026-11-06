target("ethan_game(bestoftheworld)")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "plugin-particle", "plugin-tilemap", "plugin-text", "plugin-shader", "plugin-input", "core", "contract-audiodevice", "contract-renderer", "plugin-audio", "plugin-graphics", "plugin-sprite", "plugin-transform")
    -- Built first, not linked: the Engine loads what project.luau names.
    add_deps("plugin-libraries", "backend-libraries", {inherit = false})

    add_files("src/**.cpp")
    add_includedirs("src")

    set_rundir("$(projectdir)")

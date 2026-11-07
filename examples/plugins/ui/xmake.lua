-- The UI in one game: every widget, written in Luau. Run from the repository root.
target("ui")
    set_kind("binary")
    set_default(false)
    add_deps("engine")
    -- Built first, not linked: the Engine loads what project.luau names.
    add_deps("plugin-libraries", "backend-libraries", {inherit = false})

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

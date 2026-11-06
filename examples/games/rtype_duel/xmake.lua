includes("game/plugins/greeter")

target("rtype_duel")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "core", "replication")
    -- Built first, not linked: the Engine loads what project.luau names.
    add_deps("plugin-libraries", "backend-libraries", "duel-greeter", {inherit = false})

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

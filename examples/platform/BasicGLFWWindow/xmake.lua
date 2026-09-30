target("BasicGLFWWindow")
    set_kind("binary")
    set_default(false)
    add_deps("engine", "platform")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

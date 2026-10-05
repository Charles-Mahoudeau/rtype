target("BasicGLFWWindow")
    set_kind("binary")
    set_default(false)
    add_deps("engine-core", "platform")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

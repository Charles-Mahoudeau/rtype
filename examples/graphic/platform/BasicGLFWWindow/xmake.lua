target("BasicGLFWWindow")
    set_kind("binary")
    set_default(false)
    add_deps("contract-platform", "plugin-input", "platform-glfw")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

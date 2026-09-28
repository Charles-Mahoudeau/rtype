target("BasicWindow")
    set_kind("binary")
    set_default(false)
    add_deps("vulkan")

    add_files("src/**.cpp")
    add_includedirs("$(projectdir)/src/")

    set_rundir("$(projectdir)")

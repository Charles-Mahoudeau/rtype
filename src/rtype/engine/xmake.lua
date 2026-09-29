target("engine")
    set_kind("binary")
    add_deps("luau", "vulkan")
    add_files("*.cpp")

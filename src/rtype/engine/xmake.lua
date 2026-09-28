target("engine")
    set_kind("binary")
    add_deps("lua", "vulkan")
    add_files("*.cpp")

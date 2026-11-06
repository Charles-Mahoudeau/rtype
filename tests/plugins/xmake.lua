target("plugins-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("plugin-transform", "plugin-input", "plugin-graphics", "plugin-shader", "plugin-sprite", "plugin-tilemap", "plugin-text", "plugin-particle", "plugin-audio", "plugin-physics", "plugin-physics-debug")
    add_files("*.cpp", "../main.cpp")
    -- Jolt: the physics tests include plugins/physics/detail/, which includes it.
    add_packages("gtest", "joltphysics")
    add_tests("default")

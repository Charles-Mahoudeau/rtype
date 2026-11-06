target("plugins-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("plugin-transform", "plugin-input", "plugin-graphics", "plugin-shader", "plugin-sprite", "plugin-shape", "plugin-tilemap", "plugin-text", "plugin-particle", "plugin-audio", "plugin-physics", "plugin-physics-debug")
    add_files("*.cpp", "../main.cpp")
    add_packages("gtest")
    -- The headers of Jolt only: the physics tests include plugins/physics/detail/, which includes them. Linking it too
    -- would give the process a second copy of its global state, next to the one in plugin-physics.
    add_packages("joltphysics", {links = {}})
    add_tests("default")

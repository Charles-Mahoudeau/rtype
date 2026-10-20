target("plugins-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("plugins")
    add_files("*.cpp", "../main.cpp")
    -- Jolt: the physics tests include plugins/physics/detail/, which includes it.
    add_packages("gtest", "joltphysics")
    add_tests("default")

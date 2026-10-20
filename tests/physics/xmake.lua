target("physics-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("physics")
    add_files("*.cpp", "../main.cpp")
    add_packages("gtest")
    -- Jolt's headers and defines only, for the inline conversions of JoltMath.hpp. Never its library: Jolt is static,
    -- and a second copy would have its own globals, apart from the ones rtype-physics sets up.
    add_packages("joltphysics", {links = {}})
    add_tests("default")

-- A plugin library for the PluginLoader's tests, built apart as a real one is.
target("engine-test-plugin")
    set_kind("shared")
    set_default(false)
    set_group("tests")
    set_basename("rtype-test-plugin")
    add_deps("core", "ecs")
    add_files("fixtures/TestPlugin.cpp")
    add_rules("rtype.plugin_library")

target("engine-tests")
    set_kind("binary")
    -- Built first, not linked: the Engine loads them.
    add_deps("backend-libraries", {inherit = false})
    set_default(false)
    set_group("tests")
    add_deps("engine-test-plugin", "plugin-libraries", "engine", "replication", "plugin-graphics", "plugin-input", "plugin-particle", "plugin-physics", "plugin-physics-debug", "plugin-shader", "plugin-shape", "plugin-sprite", "plugin-text", "plugin-tilemap", "plugin-transform")
    add_files("*.cpp", "../main.cpp")
    -- FakeBackend.hpp (header-only): the fake platform and renderer, for a client App that runs frames.
    add_includedirs("../plugins")
    add_packages("gtest", "libsodium")
    add_tests("default")

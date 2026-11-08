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
    add_deps("engine-test-plugin", "plugin-libraries", "engine", "replication", "plugin-capsule", "plugin-graphics", "plugin-http", "plugin-json", "plugin-input", "plugin-particle", "plugin-physics", "plugin-physics-debug", "plugin-shader", "plugin-shape", "plugin-sprite", "plugin-text", "plugin-tilemap", "plugin-transform")
    add_files("*.cpp", "../main.cpp")
    -- FakeBackend.hpp (header-only): the fake platform and renderer, for a client App that runs frames.
    add_includedirs("../plugins")
    -- ScriptProbe.hpp (header-only): the `probe` global that tests read what a script recorded through.
    add_includedirs("../scripting")
    -- asio: LocalHttpServer.hpp, the HTTP server of the http plugin's script tests.
    add_packages("gtest", "libsodium", "asio")
    -- LocalHttpServer.hpp uses Asio's sockets, as the network module does.
    if is_plat("windows") then
        -- Asio includes windows.h: no min and max macros, which break std::max and the engine's headers.
        add_defines("_WIN32_WINNT=0x0A00", "NOMINMAX", "WIN32_LEAN_AND_MEAN")
        add_syslinks("ws2_32", "mswsock")
    end
    add_tests("default")

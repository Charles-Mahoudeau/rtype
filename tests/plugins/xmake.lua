target("plugins-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("plugin-capsule", "plugin-ui", "plugin-transform", "plugin-input", "plugin-graphics", "plugin-shader", "plugin-sprite", "plugin-shape", "plugin-tilemap", "plugin-text", "plugin-particle", "plugin-audio", "plugin-physics", "plugin-physics-debug", "plugin-http")
    add_files("*.cpp", "../main.cpp")
    add_packages("gtest")
    -- LocalHttpServer.hpp: a plain HTTP server for the http plugin's tests, so none reaches the internet.
    add_packages("asio")
    -- The headers of Jolt only: the physics tests include plugins/physics/detail/, which includes them. Linking it too
    -- would give the process a second copy of its global state, next to the one in plugin-physics.
    add_packages("joltphysics", {links = {}})
    -- LocalHttpServer.hpp uses Asio's sockets, as the network module does.
    if is_plat("windows") then
        add_defines("_WIN32_WINNT=0x0A00")
        add_syslinks("ws2_32", "mswsock")
    end
    add_tests("default")

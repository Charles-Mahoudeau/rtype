-- raylib backend: a platform and a renderer on raylib, which opens its window and its OpenGL context together, so both
-- live in one library. Name them in project.luau: platform = "raylib", renderer = "raylib". The only module that
-- includes raylib.
add_requires("raylib 6.0", {system = false})

target("backend-raylib")
    add_rules("rtype.plugin_library", {folder = "plugins/backends"})
    set_kind("shared")
    set_basename("rtype-backend-raylib")
    add_deps("core", "contract-platform", "contract-renderer", {public = true})
    add_defines("RTYPE_BACKEND_RAYLIB_BUILD", {public = false})
    add_packages("raylib")
    if is_plat("linux") then
        -- raylib carries its own GLFW: keep its symbols inside this library, so they never mix with platform-glfw's
        -- in the same process.
        add_shflags("-Wl,--exclude-libs,ALL")
    end
    -- On Linux, raylib's GLFW opens the X11 libraries when it starts, and the OpenGL functions come through it. The
    -- package lists X11, OpenGL and thirty xcb libraries as links all the same, which would make this library need
    -- them to load: on a machine missing one (libxcb-xf86dri...), the engine cannot load it, even for a game that uses
    -- another backend. Only raylib is linked, and libX11, whose clipboard functions raylib calls directly; every X11 or
    -- XWayland desktop has it. platform-glfw does the same with GLFW.
    before_link(function (target)
        if target:is_plat("linux") then
            target:pkg("raylib"):set("links", {"raylib"})
            target:pkg("raylib"):set("syslinks", {"X11"})
        end
    end)
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "backend/raylib/...".
    add_includedirs("../..", {public = true})

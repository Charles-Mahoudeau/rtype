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
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "backend/raylib/...".
    add_includedirs("../..", {public = true})

-- SFML is only needed by this example: the engine itself never depends on it.
-- A system SFML is fine here (Homebrew's on macOS): unlike the SDL example, this one never includes Vulkan, so the
-- other headers its include path brings along cannot clash with the engine's. And on macOS it is needed: the 3.0.1
-- that xmake builds from source does not compile with recent Clang (fixed in SFML 3.0.2).
add_requires("sfml 3.x", {alias = "sfml", configs = {audio = false, network = false}})

target("SfmlWindow")
    set_kind("binary")
    set_default(false)
    add_deps("engine-core")
    add_packages("sfml")

    add_files("src/**.cpp")

    set_rundir("$(projectdir)")

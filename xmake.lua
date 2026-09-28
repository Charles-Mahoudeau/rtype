add_rules("mode.debug", "mode.release")
set_languages("c++23")

set_toolchains("clang")

includes("src/rtype/engine")
includes("src/rtype/lua")
includes("src/rtype/vulkan")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("engine", "lua", "vulkan")

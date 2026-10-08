add_rules("mode.debug", "mode.release")
set_languages("c++23")

if is_plat("windows") then
    set_toolchains("msvc")
else
    set_toolchains("clang")
end

-- Shared by several modules, so declared once here.
add_requires("glm 1.0.x", {alias = "glm"})
add_requires("glfw 3.4", {alias = "glfw"})
add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers"})

includes("src/rtype/ecs")
includes("src/rtype/engine")
includes("src/rtype/interop")
includes("src/rtype/platform/glfw")
includes("src/rtype/luau")
includes("src/rtype/render/vulkan")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("ecs", "engine", "platform-glfw", "luau", "render-vulkan")


local ALL_EXAMPLES_FLAG = "AllExamples"
local ALL_VULKAN_EXAMPLES_FLAG = "AllVulkanExamples"
local ALL_LUAU_EXAMPLES_FLAG = "AllLuauExamples"

option(ALL_EXAMPLES_FLAG, {default = false, description = "Enable all examples"})
option(ALL_VULKAN_EXAMPLES_FLAG, {default = false, description = "Enable all Vulkan examples"})
option(ALL_LUAU_EXAMPLES_FLAG, {default = false, description = "Enable all Luau examples"})


for _, file in ipairs(os.files(path.join(os.scriptdir(), "examples", "**", "xmake.lua"))) do
    local dir = path.directory(file)
    local name = path.basename(dir)

    option(name, {default = false, description = "Enabling \"" .. name .. "\" as an example to build."})

    if has_config(name) or has_config(ALL_EXAMPLES_FLAG) or has_config(ALL_VULKAN_EXAMPLES_FLAG)
        or has_config(ALL_LUAU_EXAMPLES_FLAG) then
        includes(file)
    end
end

local TESTS_FLAG = "Tests"

option(TESTS_FLAG, {default = false, description = "Enable unit tests"})

if has_config(TESTS_FLAG) then
    includes("tests")
end

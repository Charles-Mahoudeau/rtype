add_rules("mode.debug", "mode.release")
set_languages("c++23")

set_toolchains("clang")

-- Shared by several modules, so declared once here.
add_requires("glm 1.0.x", {alias = "glm"})
add_requires("glfw 3.4", {alias = "glfw"})
add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers"})

includes("src/rtype/engine")
includes("src/rtype/interop")
includes("src/rtype/platform")
includes("src/rtype/luau")
includes("src/rtype/vulkan")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("engine", "platform", "luau", "vulkan")


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

add_rules("mode.debug", "mode.release")
set_languages("c++23")

if is_plat("windows") then
    set_toolchains("msvc")
else
    set_toolchains("clang")
end

includes("src/rtype/engine")
includes("src/rtype/luau")
includes("src/rtype/vulkan")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("engine", "luau", "vulkan")


local ALL_EXAMPLES_FLAG = "AllExamples"
local ALL_VULKAN_EXAMPLES_FLAG = "AllVulkanExamples"
local ALL_LUAU_EXAMPLES_FLAG = "AllLuauExamples"

option(ALL_EXAMPLES_FLAG, {default = false, description = "Enable all examples"})
option(ALL_VULKAN_EXAMPLES_FLAG, {default = false, description = "Enable all Vulkan examples"})
option(ALL_LUAU_EXAMPLES_FLAG, {default = false, description = "Enable all Luau examples"})

local all_examples_folder = {}

table.join2(all_examples_folder, os.dirs(path.join(".", "examples", "*", "*")))

for _, dir in ipairs(all_examples_folder) do
    local name = path.basename(dir)

    option(name, {default = false, description = "Enabling \"" .. name .. "\" as an example to build."})

    if has_config(ALL_EXAMPLES_FLAG) or has_config(name) or has_config(ALL_LUAU_EXAMPLES_FLAG) or has_config(ALL_VULKAN_EXAMPLES_FLAG) then
        if (not os.isfile(path.join(dir, "xmake.lua"))) then
            print("Warning: No xmake.lua found in " .. dir .. ", skipping example...")
        else
            if has_config(name) then
                includes(path.join(dir, "xmake.lua"))
            elseif has_config(ALL_LUAU_EXAMPLES_FLAG) then
                includes(path.join(dir, "xmake.lua"))
            elseif has_config(ALL_VULKAN_EXAMPLES_FLAG) then
                includes(path.join(dir, "xmake.lua"))
            elseif has_config(ALL_EXAMPLES_FLAG) then
                includes(path.join(dir, "xmake.lua"))
            end
        end
    end
end

local TESTS_FLAG = "Tests"

option(TESTS_FLAG, {default = false, description = "Enable unit tests"})

if has_config(TESTS_FLAG) then
    includes("tests")
end

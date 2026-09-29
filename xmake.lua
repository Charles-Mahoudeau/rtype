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


local ALL_EXAMPLES_FLAG = "AllExamples"
local ALL_VULKAN_EXAMPLES_FLAG = "AllVulkanExamples"
local ALL_LUA_EXAMPLES_FLAG = "AllLuaExamples"

option(ALL_EXAMPLES_FLAG, {default = false, description = "Enable all examples"})
option(ALL_VULKAN_EXAMPLES_FLAG, {default = false, description = "Enable all Vulkan examples"})
option(ALL_LUA_EXAMPLES_FLAG, {default = false, description = "Enable all Lua examples"})

local all_examples_folder = {}

table.join2(all_examples_folder, os.dirs(path.join(".", "examples", "*", "*")))

for _, dir in ipairs(all_examples_folder) do
    local name = path.basename(dir)

    option(name, {default = false, description = "Enabling \"" .. name .. "\" as an example to build."})

    if has_config(ALL_EXAMPLES_FLAG) or has_config(name) or has_config(ALL_LUA_EXAMPLES_FLAG) or has_config(ALL_VULKAN_EXAMPLES_FLAG) then
        if (not os.isfile(path.join(dir, "xmake.lua"))) then
            print("Warning: No xmake.lua found in " .. dir .. ", skipping example...")
        else
            if has_config(name) then
                includes(path.join(dir, "xmake.lua"))
            elseif has_config(ALL_LUA_EXAMPLES_FLAG) then
                includes(path.join(dir, "xmake.lua"))
            elseif has_config(ALL_VULKAN_EXAMPLES_FLAG) then
                includes(path.join(dir, "xmake.lua"))
            elseif has_config(ALL_EXAMPLES_FLAG) then
                includes(path.join(dir, "xmake.lua"))
            end
        end
    end
end
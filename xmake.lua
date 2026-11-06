add_rules("mode.debug", "mode.release")
set_languages("c++23")

if is_plat("windows") then
    set_toolchains("msvc")
else
    set_toolchains("clang")
end

-- xmake turns its build cache off for MSVC (cl.exe's preprocessor is slow), so the CI rebuilt everything on Windows
-- every time. The CI sets XMAKE_ACTION_BUILD_CACHE: there, the cache is on for every compiler, MSVC included.
if os.getenv("XMAKE_ACTION_BUILD_CACHE") == "true" then
    set_policy("build.ccache", true)
end

-- Shared by several modules, so declared once here.
add_requires("glm 1.0.x", {alias = "glm"})
add_requires("glfw 3.4", {alias = "glfw"})
add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers"})
add_requires("stb", {alias = "stb"})
-- glslang: the library compiles the game's shaders at runtime (plugins), glslangValidator the renderer's built-in ones
-- (render-vulkan, vulkan-tests). One requirement for both, as xmake installs one configuration of a package: two
-- would leave one side without what it needs. On macOS, the library alone: the tool and SPIRV-Tools need xmake's own
-- Python, which no longer builds there (its _ssl module fails to link), so glslangValidator comes from Homebrew.
if is_plat("macosx") then
    add_requires("glslang 1.4.x", {alias = "glslang", configs = {default_resource_limits = true}})
else
    add_requires("glslang 1.4.x",
                 {alias = "glslang", configs = {tools = true, spirv_tools = true, default_resource_limits = true}})
end

-- Copies a binary to the repository root after it is built, where the Epitech subject expects r-type_client and
-- r-type_server. The copy finds the project's shared libraries where they were built: through a relative rpath on
-- Linux and macOS, and, on Windows (no rpath), through copies of the DLLs of the build folder next to it.
rule("rtype.copy_to_root")
    on_load(function (target)
        if not target:is_plat("windows") then
            target:add("rpathdirs", path.join("@loader_path", path.relative(target:targetdir(), os.projectdir())))
        end
    end)
    after_build(function (target)
        os.cp(target:targetfile(), os.projectdir())
        if target:is_plat("windows") then
            os.cp(path.join(target:targetdir(), "*.dll"), os.projectdir())
        end
    end)
rule_end()

includes("src/rtype/core")
includes("src/rtype/ecs")
includes("src/rtype/scripting")
includes("src/rtype/network")
includes("src/rtype/replication")
includes("src/rtype/engine")
includes("src/rtype/contract")
includes("src/rtype/plugins")
includes("src/rtype/interop")
includes("src/rtype/platform/glfw")
includes("src/rtype/luau")
includes("src/rtype/render/vulkan")
includes("src/rtype/audio/miniaudio")
includes("src/rtype/client")
includes("src/rtype/server")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("core", "ecs", "network", "replication", "engine", "platform-glfw", "luau", "render-vulkan", "audio-miniaudio",
             "r-type_client", "r-type_server")


local ALL_EXAMPLES_FLAG = "AllExamples"
local ALL_VULKAN_EXAMPLES_FLAG = "AllVulkanExamples"
local ALL_LUAU_EXAMPLES_FLAG = "AllLuauExamples"
local ALL_ECS_EXAMPLES_FLAG = "AllEcsExamples"

option(ALL_EXAMPLES_FLAG, {default = false, description = "Enable all examples"})
option(ALL_VULKAN_EXAMPLES_FLAG, {default = false, description = "Enable all Vulkan examples"})
option(ALL_LUAU_EXAMPLES_FLAG, {default = false, description = "Enable all Luau examples"})
option(ALL_ECS_EXAMPLES_FLAG, {default = false, description = "Enable all ECS examples"})


for _, file in ipairs(os.files(path.join(os.scriptdir(), "examples", "**", "xmake.lua"))) do
    local dir = path.directory(file)
    local name = path.basename(dir)

    option(name, {default = false, description = "Enabling \"" .. name .. "\" as an example to build."})

    if has_config(name) or has_config(ALL_EXAMPLES_FLAG) or has_config(ALL_VULKAN_EXAMPLES_FLAG)
        or has_config(ALL_LUAU_EXAMPLES_FLAG) or has_config(ALL_ECS_EXAMPLES_FLAG) then
        includes(file)
    end
end

local TESTS_FLAG = "Tests"

option(TESTS_FLAG, {default = false, description = "Enable unit tests"})

if has_config(TESTS_FLAG) then
    includes("tests")
end

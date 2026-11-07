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

-- Makes a shared library a plugin library: it appears in the `plugins` folder next to the executables (or the folder
-- `rtype.plugin_folder` names), where the Engine finds it. The folder links to the library rather than holding a second
-- file, which would load twice; on Windows, where SharedLibrary shares the modules of the same name, it is a copy.
rule("rtype.plugin_library")
    on_load(function (target)
        if not target:is_plat("windows") then
            -- Opened through the folder, it finds what it links one level up.
            target:add("rpathdirs", "@loader_path/..", "@loader_path/../..")
        end
    end)
    before_link(function (target)
        -- Linked by the executable, a package's libraries were found through it; opened alone, the library needs them.
        if target:is_plat("linux") then
            for _, dirs in ipairs(target:get_from("linkdirs", "package::*") or {}) do
                target:add("rpathdirs", dirs)
            end
        end
    end)
    after_build(function (target)
        local folder = path.join(target:targetdir(), target:extraconf("rules", "rtype.plugin_library", "folder") or "plugins")
        local file = path.join(folder, path.filename(target:targetfile()))
        os.mkdir(folder)
        os.tryrm(file)
        if target:is_plat("windows") then
            os.cp(target:targetfile(), file)
        else
            os.ln(path.absolute(target:targetfile()), file)
        end
    end)
rule_end()

-- Makes a target a game's own plugin library: a folder `<game>/plugins/<name>/`, with its xmake.lua and its sources, built
-- into `<game>/plugins/`, where the Engine finds it. Nothing of the engine changes for it. A game's xmake.lua (or the
-- root's scan of `game/plugins/`) includes that file.
rule("rtype.game_plugin")
    on_load(function (target)
        -- The library stays in the game: it links the engine's libraries through their folder.
        local folder = path.directory(target:scriptdir())
        target:set("targetdir", folder)
        if not target:is_plat("windows") then
            local build = path.join(os.projectdir(), "build", target:plat(), target:arch(), get_config("mode") or "release")
            target:add("rpathdirs", path.join("@loader_path", path.relative(build, folder)))
        end
    end)
rule_end()

-- Copies a binary to the repository root after it is built
rule("rtype.copy_to_root")
    on_load(function (target)
        if not target:is_plat("windows") then
            target:add("rpathdirs", path.join("@loader_path", path.relative(target:targetdir(), os.projectdir())))
        end
    end)
    after_build(function (target)
        os.cp(target:targetfile(), os.projectdir())
        -- The plugin libraries, where the Engine looks for them: next to the executable.
        local plugins = path.join(os.projectdir(), "plugins")
        os.tryrm(plugins)
        if target:is_plat("windows") then
            os.cp(path.join(target:targetdir(), "plugins"), plugins)
        else
            os.ln(path.absolute(path.join(target:targetdir(), "plugins")), plugins)
        end
        if target:is_plat("windows") then
            -- No rpath there, so its DLLs go next to it. Only those it depends on: the others may still be linking, and
            -- Windows refuses to read a file being written.
            for _, dep in ipairs(target:orderdeps()) do
                if dep:kind() == "shared" then
                    os.cp(dep:targetfile(), os.projectdir())
                end
            end
        end
    end)
rule_end()

-- The game's own plugins, if the repository has a game.
for _, file in ipairs(os.files(path.join(os.scriptdir(), "game", "plugins", "*", "xmake.lua"))) do
    includes(file)
end

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
includes("src/rtype/runtime")
includes("src/rtype/tools/sign")

-- Every backend library, for what loads them without linking them.
target("backend-libraries")
    set_kind("phony")
    add_deps("platform-glfw", "render-vulkan", "audio-miniaudio")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("core", "ecs", "network", "replication", "engine", "platform-glfw", "luau", "render-vulkan", "audio-miniaudio",
             "aylo-runtime")


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

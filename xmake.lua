add_rules("mode.debug", "mode.release")
set_languages("c++23")

-- The version of the engine, the only place it is written: `aylo-runtime --version` and the release package read it
-- (xmake/modules/engine_version.lua). SemVer; 0.x until the script API settles (docs/release/plan.md).
set_version("0.1.0")
-- A nightly build: `xmake f --nightly=<commit>` makes the version `<version>-nightly.<commit>`.
option("nightly", {default = "", showmenu = true, description = "Commit a nightly build is made from; empty for a release"})
add_moduledirs(path.join(os.scriptdir(), "xmake", "modules"))
-- `xmake package-release`: the zip game developers download.
includes("xmake/package-release.lua")

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

if is_plat("windows") then
    -- The dynamic C runtime, shared by every library of the engine (C++ types cross their boundaries). Its DLLs go in
    -- the release folder next to the executable.
    set_runtimes(is_mode("debug") and "MDd" or "MD")
end

-- Shared by several modules, so declared once here.
add_requires("glm 1.0.x", {alias = "glm"})
add_requires("glfw 3.4", {alias = "glfw"})
-- Never the system's: apt's are older than 1.4 and lack vulkan_hpp_macros.hpp, which vulkan-memory-allocator-hpp needs.
add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers", system = false})
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
    -- Its SPIRV-Tools are the ones xmake builds, static: the system's are shared libraries the release does not hold.
    add_requireconfs("glslang.spirv-tools", {system = false, override = true})
end

-- The layout of what is built, the same in the build folder and in the release zip (docs/release/package.md):
--
--   aylo-runtime            the executables (and the tests) at the root of the folder
--   lib/                    the shared libraries that are not plugins: `ecs`, the contracts, `engine`...
--   plugins/                the plugin libraries, real files; the Engine scans it
--   plugins/backends/       the platform, renderer and audio backends; the Engine scans it too
--
-- A library knows where its dependencies are from its own folder (rpath on Linux and macOS, all inside the folder).
-- Windows has no rpath: the DLLs an executable links are copied next to it, and SharedLibrary adds `lib/` to the folders
-- a plugin's DLLs are looked for in.
rule("rtype.layout")
    on_load(function (target)
        local kind = target:kind()
        if (kind ~= "shared" and kind ~= "binary") or target:rule("rtype.plugin_library") or target:rule("rtype.game_plugin") then
            return
        end
        if kind == "shared" then
            target:set("targetdir", path.join(target:targetdir(), "lib"))
            if not target:is_plat("windows") then
                target:add("rpathdirs", "@loader_path")
            end
        elseif not target:is_plat("windows") then
            -- `plugins/` for the plugin libraries an executable links (the engine's transform and input plugins).
            target:add("rpathdirs", "@loader_path/lib", "@loader_path/plugins")
        end
    end)
    after_build(function (target)
        if target:kind() == "binary" and target:is_plat("windows") then
            -- Only the DLLs it depends on: the others may still be linking, and Windows refuses to read a file being written.
            for _, dep in ipairs(target:orderdeps()) do
                if dep:kind() == "shared" then
                    os.cp(dep:targetfile(), target:targetdir())
                end
            end
        end
    end)
rule_end()
add_rules("rtype.layout")

-- Makes a shared library a plugin library: it is built into the `plugins` folder next to the executables (or the folder
-- `folder` names, `plugins/backends` for a backend), where the Engine finds it. It is the only copy of the library:
-- the plugins it links are in its own folder, the other libraries in `lib/`.
rule("rtype.plugin_library")
    on_load(function (target)
        local folder = target:extraconf("rules", "rtype.plugin_library", "folder") or "plugins"
        target:set("targetdir", path.join(target:targetdir(), folder))
        if not target:is_plat("windows") then
            -- A plugin finds the plugins it links beside it, and `lib/` one folder up; a backend, one folder deeper, only
            -- `lib/`. Every entry stays inside the release folder, which is what `.github/scripts/check-self-contained.sh`
            -- checks.
            local up = "@loader_path"
            for _ = 1, #path.split(folder) do
                up = up .. "/.."
            end
            if folder == "plugins" then
                target:add("rpathdirs", "@loader_path")
            end
            target:add("rpathdirs", up .. "/lib")
        end
    end)
rule_end()

-- Makes a target a game's own plugin library: a folder `<game>/plugins/<name>/`, with its xmake.lua and its sources, built
-- into `<game>/plugins/`, where the Engine finds it. Nothing of the engine changes for it. A game's xmake.lua (or the
-- root's scan of `game/plugins/`) includes that file.
rule("rtype.game_plugin")
    on_load(function (target)
        -- The library stays in the game: it links the engine's libraries through their folders.
        local folder = path.directory(target:scriptdir())
        target:set("targetdir", folder)
        if not target:is_plat("windows") then
            local build = path.join(os.projectdir(), "build", target:plat(), target:arch(), get_config("mode") or "release")
            target:add("rpathdirs", path.join("@loader_path", path.relative(path.join(build, "lib"), folder)),
                       path.join("@loader_path", path.relative(path.join(build, "plugins"), folder)))
        end
    end)
rule_end()

-- Copies a binary to the repository root after it is built
rule("rtype.copy_to_root")
    on_load(function (target)
        if not target:is_plat("windows") then
            -- The build folder, where the binary finds its libraries and plugins before it is packaged.
            local build = path.join("@loader_path", path.relative(target:targetdir(), os.projectdir()))
            target:add("rpathdirs", path.join(build, "lib"), path.join(build, "plugins"))
        end
    end)
    after_build(function (target)
        -- Removed first, so the copy is a new file: macOS caches the code signature of a file it ran, and kills a binary
        -- written over it in place ("Code Signature Invalid") the next time it starts.
        os.tryrm(path.join(os.projectdir(), path.filename(target:targetfile())))
        os.cp(target:targetfile(), os.projectdir())
        -- The folders next to the executable, where the Engine looks for the plugins: links to those of the build, which
        -- are real files (a copy on Windows, which has no link it can count on).
        for _, folder in ipairs({"plugins", "lib"}) do
            local from = path.join(target:targetdir(), folder)
            local to = path.join(os.projectdir(), folder)
            os.tryrm(to)
            if target:is_plat("windows") then
                os.cp(from, to)
            elseif folder == "plugins" then
                os.ln(path.absolute(from), to)
            end
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

-- The modules that are static libraries because no plugin links them: they live in the executable, once. A shared library
-- (a plugin, a backend, a contract) linking one would carry its own copy, and the state of the two copies would drift
-- apart (docs/Modules.md). The build refuses it. `core` is static too, but stateless: any library may link it.
rule("rtype.static_modules")
    on_config(function (target)
        local executableOnly = {luau = true, network = true, replication = true, scripting = true, engine = true}
        if target:kind() ~= "shared" then
            return
        end
        for _, dep in ipairs(target:orderdeps()) do
            if executableOnly[dep:name()] then
                raise("%s is a shared library and links %s, a static module only an executable may link: " ..
                      "its state would be duplicated (see docs/Modules.md)", target:name(), dep:name())
            end
        end
    end)
rule_end()
add_rules("rtype.static_modules")

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
includes("src/rtype/editor")
includes("src/rtype/interop")
includes("src/rtype/platform/glfw")
includes("src/rtype/backend/null")
includes("src/rtype/luau")
includes("src/rtype/render/vulkan")
includes("src/rtype/audio/miniaudio")
includes("src/rtype/runtime")
includes("src/rtype/tools/sign")

-- Every backend library, for what loads them without linking them.
target("backend-libraries")
    set_kind("phony")
    add_deps("platform-glfw", "render-vulkan", "audio-miniaudio", "backend-null")

target("rtype")
    set_kind("phony")
    set_default(true)
    add_deps("core", "ecs", "network", "replication", "engine", "platform-glfw", "luau", "render-vulkan", "audio-miniaudio", "backend-null",
             "aylo-runtime", "aylo-editor")


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

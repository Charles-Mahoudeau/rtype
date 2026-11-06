-- Plugins: the engine as ECS plugins, one shared library per plugin (rtype-plugin-<name>). Each one keeps its own code
-- and declares its own components, resources and events. It links the plugins whose code it uses, and names the ones
-- it requires in getDependencies(): no plugin adds another.

-- Keep debug_renderer at its default: that option does not reach Jolt's CMake (the library is always built with
-- JPH_DEBUG_RENDERER), and a define that differs from the library's makes JPH::RegisterTypes() abort.
-- 32-bit object layers: each body's layer holds a static flag, its collision categories and its mask (1 + 15 + 15 bits,
-- see physics/detail/Layers). This option does reach Jolt's CMake, and the package adds the matching define for us.
add_requires("joltphysics 5.6.x", {alias = "joltphysics", configs = {object_layer_bits = "32"}})
add_requires("nlohmann_json 3.12.x", {alias = "nlohmann_json"})

-- Starts the target of the plugin `name` and what every plugin shares. `deps`: the libraries its public headers use.
local function plugin(name, deps)
    target("plugin-" .. name)
        set_kind("shared")
        set_basename("rtype-plugin-" .. name)
        add_defines("RTYPE_PLUGIN_" .. name:upper():gsub("-", "_") .. "_BUILD", {public = false})
        add_deps("core", "ecs", {public = true})
        for _, dep in ipairs(deps) do
            add_deps(dep, {public = true})
        end
        -- Headers are included as "plugins/...".
        add_includedirs("..", {public = true})
        add_packages("glm", {public = true})
        -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
        add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
end

plugin("transform", {})
    add_files("transform/**.cpp")
    add_headerfiles("transform/**.hpp", "transform/**.tpp")

plugin("input", {"contract-platform"})
    add_files("input/**.cpp")
    add_headerfiles("input/**.hpp", "input/**.tpp")

plugin("graphics", {"contract-renderer", "plugin-input", "plugin-transform"})
    add_files("graphics/**.cpp")
    add_headerfiles("graphics/**.hpp", "graphics/**.tpp")
    -- Decodes image files into texture pixels (graphics/ImageDecoding.cpp). Private.
    add_packages("stb")

plugin("shader", {"plugin-graphics"})
    add_files("shader/**.cpp")
    add_headerfiles("shader/**.hpp", "shader/**.tpp")
    -- The shaders' compiler (glslang, required by the root xmake.lua). Private: only shader/detail/ShaderCompiler.cpp
    -- includes it.
    add_packages("glslang")
    -- The shaders ABI of the renderer contract (contract/renderer/shaders/*.glsl), the prelude of the game's shaders,
    -- embedded as byte arrays that shader/detail/ShaderAssembler.cpp includes as "<file>.glsl.h".
    add_rules("utils.bin2c", {extensions = {".glsl"}})
    add_files("../contract/renderer/shaders/*.glsl")

plugin("sprite", {"plugin-graphics", "plugin-shader", "plugin-transform"})
    add_files("sprite/**.cpp")
    add_headerfiles("sprite/**.hpp", "sprite/**.tpp")
    -- Reads Aseprite and TexturePacker atlases, and packs images with stb_rect_pack. Private: only sprite/detail/
    -- includes them.
    add_packages("nlohmann_json", "stb")

plugin("tilemap", {"plugin-graphics", "plugin-transform"})
    add_files("tilemap/**.cpp")
    add_headerfiles("tilemap/**.hpp", "tilemap/**.tpp")
    -- Reads Tiled maps. Private: only tilemap/detail/ includes it.
    add_packages("nlohmann_json")

plugin("text", {"plugin-graphics", "plugin-shader", "plugin-transform"})
    add_files("text/**.cpp")
    add_headerfiles("text/**.hpp", "text/**.tpp")
    -- Reads msdf-atlas-gen fonts. Private: only text/detail/ includes it.
    add_packages("nlohmann_json")

plugin("particle", {"plugin-graphics", "plugin-shader", "plugin-transform"})
    add_files("particle/**.cpp")
    add_headerfiles("particle/**.hpp", "particle/**.tpp")

plugin("audio", {"contract-audiodevice", "plugin-transform"})
    add_files("audio/**.cpp")
    add_headerfiles("audio/**.hpp", "audio/**.tpp")

plugin("physics", {"plugin-transform"})
    add_files("physics/**.cpp|debug/**.cpp")
    add_headerfiles("physics/**.hpp|debug/**.hpp", "physics/**.tpp")
    -- The physics engine. Private: only physics .cpp files and physics/detail/ include Jolt.
    add_packages("joltphysics")

plugin("physics-debug", {"plugin-physics", "plugin-graphics", "plugin-input", "plugin-transform"})
    add_files("physics/debug/**.cpp")
    add_headerfiles("physics/debug/**.hpp")

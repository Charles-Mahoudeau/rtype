-- Plugins: the engine as ECS plugins, one folder per plugin. Each plugin keeps its own code (the backend plugin also
-- holds the contracts the platforms and renderers implement: IPlatform, IRenderer, Event, ...) and declares
-- its own components, resources and events.

-- Keep debug_renderer at its default: that option does not reach Jolt's CMake (the library is always built with
-- JPH_DEBUG_RENDERER), and a define that differs from the library's makes JPH::RegisterTypes() abort.
-- 32-bit object layers: each body's layer holds a static flag, its collision categories and its mask (1 + 15 + 15 bits,
-- see physics/detail/Layers). This option does reach Jolt's CMake, and the package adds the matching define for us.
add_requires("joltphysics 5.6.x", {alias = "joltphysics", configs = {object_layer_bits = "32"}})
add_requires("nlohmann_json 3.12.x", {alias = "nlohmann_json"})

target("plugins")
    set_kind("static")
    set_basename("rtype-plugins")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Headers are included as "plugins/...".
    add_includedirs("..", {public = true})
    add_deps("core", "ecs", {public = true})
    add_packages("glm", {public = true})
    -- Decodes image files into texture pixels (backend/graphics/ImageDecoding.cpp) and packs atlases
    -- (sprite/detail/AtlasPacker.cpp).
    add_packages("stb")
    -- Reads Aseprite and TexturePacker atlases (sprite/). Private: only sprite/detail/AtlasJson.cpp includes it.
    add_packages("nlohmann_json")
    -- The physics engine (physics/). Private: only physics .cpp files and physics/detail/ include Jolt.
    add_packages("joltphysics")
    -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Linked into the shared modules (render-vulkan).
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})

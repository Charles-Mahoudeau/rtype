-- Plugins: the engine as ECS plugins, one folder per plugin. Each plugin keeps its own code (the backend plugin also
-- holds the contracts the platforms and renderers implement: IPlatform, IRenderer, Event, Settings...) and declares
-- its own components, resources and events.

-- Keep the package's defaults: its debug_renderer option does not reach Jolt's CMake (the library is always built with
-- JPH_DEBUG_RENDERER), and a define that differs from the library's makes JPH::RegisterTypes() abort.
add_requires("joltphysics 5.6.x", {alias = "joltphysics"})

target("plugins")
    set_kind("static")
    set_basename("rtype-plugins")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Headers are included as "plugins/...".
    add_includedirs("..", {public = true})
    add_deps("ecs", {public = true})
    add_packages("glm", {public = true})
    -- Decodes image files into texture pixels (backend/graphics/TextureAssets.cpp).
    add_packages("stb")
    -- The physics engine (physics/). Private: only physics .cpp files and physics/detail/ include Jolt.
    add_packages("joltphysics")
    -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Linked into the shared modules (render-vulkan).
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})

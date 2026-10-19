-- Plugins: the engine as ECS plugins, one folder per plugin. Each plugin keeps its own code (the backend plugin also
-- holds the contracts the platforms and renderers implement: IPlatform, IRenderer, Event, Settings...) and declares
-- its own components, resources and events.
target("plugins")
    set_kind("static")
    set_basename("rtype-plugins")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Headers are included as "plugins/...".
    add_includedirs("..", {public = true})
    add_deps("ecs", {public = true})
    add_packages("glm", {public = true})
    -- Decodes image files into texture pixels (backend/TextureAssets.cpp).
    add_packages("stb")
    -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
    add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
    -- Linked into the shared modules (render-vulkan).
    add_cxflags("-fPIC", {tools = {"clang", "gcc"}})

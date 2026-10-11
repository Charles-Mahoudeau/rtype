add_requires("tl_expected")

target("ecs")
    set_kind("shared")
    set_basename("rtype-ecs")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_ECS_BUILD", {public = false})
    -- Headers are included as "rtype/ecs/...".
    add_includedirs("$(projectdir)/src", {public = true})
    -- Public: Result (tl::expected) appears in the public headers.
    add_packages("tl_expected", {public = true})
    -- RTYPE_ECS_COMPONENT relies on __VA_OPT__, which MSVC only supports with its conforming preprocessor.
    -- Public: the macro expands in the files of every module that declares components.
    add_cxflags("/Zc:preprocessor", {tools = {"cl"}, public = true})

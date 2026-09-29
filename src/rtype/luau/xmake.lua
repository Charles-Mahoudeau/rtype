add_requires("luau")

target("luau")
    set_kind("shared")
    set_basename("rtype-luau")
    add_files("**.cpp")
    add_defines("RTYPE_LUAU_BUILD", {public = false})
    add_includedirs("$(projectdir)/src", {public = true})
    add_packages("luau", {public = false})

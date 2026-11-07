add_requires("asio 1.36.x", {alias = "asio"})

target("network")
    -- Static, linked into the executable: no plugin links it (rtype.static_modules enforces it).
    set_kind("static")
    set_basename("rtype-network")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_NETWORK_BUILD", {public = false})
    add_defines("RTYPE_NETWORK_STATIC", {public = true})
    -- Headers are included as "rtype/network/...".
    add_includedirs("$(projectdir)/src", {public = true})
    -- Private: no public header includes Asio, so its users never see it.
    add_packages("asio", {public = false})
    if is_plat("windows") then
        add_defines("_WIN32_WINNT=0x0A00", {public = false})
        add_syslinks("ws2_32", "mswsock", {public = false})
    end

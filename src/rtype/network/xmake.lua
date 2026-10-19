add_requires("asio 1.36.x", {alias = "asio"})

target("network")
    set_kind("shared")
    set_basename("rtype-network")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("RTYPE_NETWORK_BUILD", {public = false})
    -- Headers are included as "rtype/network/...".
    add_includedirs("$(projectdir)/src", {public = true})
    -- Private: no public header includes Asio, so its users never see it.
    add_packages("asio", {public = false})
    if is_plat("windows") then
        add_defines("_WIN32_WINNT=0x0A00", {public = false})
        add_syslinks("ws2_32", "mswsock", {public = false})
    end

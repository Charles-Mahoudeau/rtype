-- rtype-sign: makes the keys and signatures of the game plugins (docs/Plugins.md).
target("rtype-sign")
    set_kind("binary")
    set_default(false)
    add_deps("engine")
    add_packages("libsodium")
    add_files("main.cpp")

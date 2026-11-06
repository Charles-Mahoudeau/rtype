-- Contracts: the interfaces the implementations (platform-*, render-*, audio-*) provide and the plugins use, with the
-- vocabulary they speak. One small library per interface, so a program links only what it needs. A contract knows no
-- ECS and no plugin: what a plugin needs from it goes through its own declarations (RTYPE_ECS_* in the plugin).

-- Declares the contract `name`, in the folder of the same name.
local function contract(name)
    target("contract-" .. name)
        set_kind("shared")
        set_basename("rtype-contract-" .. name)
        add_files(name .. "/**.cpp")
        add_headerfiles(name .. "/**.hpp")
        add_defines("RTYPE_CONTRACT_" .. name:upper() .. "_BUILD", {public = false})
        add_deps("core", {public = true})
        -- Headers are included as "rtype/contract/...".
        add_includedirs("$(projectdir)/src", {public = true})
        add_packages("glm", {public = true})
        -- Plain x/y/z/w members instead of unions (cppcoreguidelines-pro-type-union-access).
        add_defines("GLM_FORCE_XYZW_ONLY", {public = true})
end

contract("platform")

contract("renderer")
    -- IRenderer takes an IPlatform, and BackendRegistry starts the pair.
    add_deps("contract-platform", {public = true})

contract("audiodevice")

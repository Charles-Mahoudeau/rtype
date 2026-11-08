-- SDL3 platform: the window and the input devices, through SDL3. The only module that includes SDL.
-- Built by xmake (system = false): a system SDL3 brings its whole prefix as include path (/opt/homebrew/include with
-- Homebrew), Vulkan headers of another version included.
add_requires("libsdl3", {alias = "sdl3", system = false})

target("platform-sdl")
    add_rules("rtype.plugin_library", {folder = "plugins/backends"})
    set_kind("shared")
    set_basename("rtype-platform-sdl")
    add_defines("RTYPE_PLATFORM_SDL_BUILD", {public = false})
    -- Public: Registration.hpp includes PlatformRegistry.
    add_deps("core", "contract-platform", {public = true})
    add_packages("sdl3")
    -- SDL opens the X11 and Wayland libraries when it starts; only SDL itself is linked, so a machine without a
    -- display server (the headless server) still loads the library.
    before_link(function (target)
        if target:is_plat("linux") then
            target:pkg("sdl3"):set("links", {"SDL3"})
        end
    end)
    -- SdlPlatform implements IVulkanSurfaceSource. Public: SdlPlatform.hpp includes it.
    add_deps("interop-vulkan", {public = true})
    -- dladdr(), to find the file of the Vulkan loader (older glibc keeps it in libdl).
    if is_plat("linux") then
        add_syslinks("dl")
    end
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    -- Headers are included as "platform/sdl/...".
    add_includedirs("../..", {public = true})

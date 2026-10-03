-- SDL3 is only needed by this example: the engine itself never depends on it.
-- Built by xmake (system = false): a system SDL3 brings its whole prefix as include path (/opt/homebrew/include
-- with Homebrew), Vulkan headers of another version included, which would break vk::raii (VK_HEADER_VERSION).
add_requires("libsdl3", {alias = "sdl3", system = false})

target("SdlVulkanWindow")
    set_kind("binary")
    set_default(false)
    add_deps("engine-core", "vulkan")
    add_packages("sdl3")

    add_files("src/**.cpp")
    -- dladdr(), to find the file of the Vulkan loader (older glibc keeps it in libdl).
    if is_plat("linux") then
        add_syslinks("dl")
    end

    set_rundir("$(projectdir)")

add_requires("vulkan-headers 1.4.x", {alias = "vulkan-headers"})
add_requires("vulkan-loader 1.4.x", {alias = "vulkan-loader"})

target("vulkan")
    set_kind("shared")
    add_packages("vulkan-headers", "vulkan-loader")
    add_files("*.cpp")

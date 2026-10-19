target("vulkan-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("render-vulkan", "plugins")
    add_files("*.cpp", "../main.cpp")
    -- A test shader, embedded by the same rule as the renderer's built-in shaders.
    add_rules("glsl.spirv")
    add_files("shaders/*.vert")
    -- The renderer's shader headers, so a test shader checks they compile together (ShaderLayout.glsl).
    set_values("glsl.includedirs", path.join(os.projectdir(), "src/rtype/render/vulkan/shaders/include"))
    add_packages("gtest", "glslang")
    add_tests("default")

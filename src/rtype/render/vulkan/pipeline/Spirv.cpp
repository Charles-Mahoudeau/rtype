/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Spirv
*/

#include "Spirv.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "render/vulkan/exceptions/ShaderException.hpp"

namespace rtype::render::vulkan::pipeline {

void validateSpirv(std::span<const std::uint32_t> code, std::string_view name) {
    if (code.empty()) {
        throw exceptions::ShaderException(std::format("Shader '{}' is empty", name));
    }
    if (code.front() != kSpirvMagic) {
        throw exceptions::ShaderException(
            std::format("Shader '{}' is not SPIR-V (first word {:#010x}, expected {:#010x}): compile it with "
                        "glslangValidator -V",
                        name, code.front(), kSpirvMagic));
    }
}

std::vector<std::uint32_t> readSpirv(const std::filesystem::path& path) {
    const std::string name = path.string();
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) {
        throw exceptions::ShaderException(std::format("Shader file not found: '{}'", name));
    }
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    if (error) {
        throw exceptions::ShaderException(std::format("Cannot read shader '{}': {}", name, error.message()));
    }
    if (size % sizeof(std::uint32_t) != 0) {
        throw exceptions::ShaderException(
            std::format("Shader '{}' is not SPIR-V: its size ({} bytes) is not a multiple of 4", name, size));
    }

    std::ifstream file{path, std::ios::binary};
    std::vector<char> bytes(static_cast<std::size_t>(size));
    if (!file || !file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) {
        throw exceptions::ShaderException(std::format("Cannot read shader '{}'", name));
    }
    std::vector<std::uint32_t> code(bytes.size() / sizeof(std::uint32_t));
    std::memcpy(code.data(), bytes.data(), bytes.size());
    validateSpirv(code, name);
    return code;
}

}  // namespace rtype::render::vulkan::pipeline

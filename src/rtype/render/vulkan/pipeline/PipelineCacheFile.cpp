/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineCacheFile
*/

#include "PipelineCacheFile.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::pipeline {

namespace {

/// @brief The fields of VkPipelineCacheHeaderVersionOne, read from raw bytes.
struct Header {
    std::uint32_t size = 0;                         ///< headerSize.
    std::uint32_t version = 0;                      ///< headerVersion.
    std::uint32_t vendorId = 0;                     ///< vendorID.
    std::uint32_t deviceId = 0;                     ///< deviceID.
    std::array<std::uint8_t, VK_UUID_SIZE> uuid{};  ///< pipelineCacheUUID.
};

/// @brief Bytes of a VkPipelineCacheHeaderVersionOne: four uint32 fields, then the UUID.
constexpr std::size_t kHeaderSize = (4 * sizeof(std::uint32_t)) + VK_UUID_SIZE;

/// @return The uint32 at @p offset of @p data, as the device wrote it (its own byte order).
std::uint32_t readUint32(std::span<const std::byte> data, std::size_t offset) noexcept {
    std::uint32_t value = 0;
    std::memcpy(&value, data.subspan(offset, sizeof(value)).data(), sizeof(value));
    return value;
}

}  // namespace

bool isCompatiblePipelineCache(std::span<const std::byte> data,
                               const vk::PhysicalDeviceProperties& properties) noexcept {
    if (data.size() < kHeaderSize) {
        return false;
    }
    Header header{.size = readUint32(data, 0),
                  .version = readUint32(data, 4),
                  .vendorId = readUint32(data, 8),
                  .deviceId = readUint32(data, 12)};
    std::memcpy(header.uuid.data(), data.subspan(16, VK_UUID_SIZE).data(), VK_UUID_SIZE);
    return header.size == kHeaderSize &&
           header.version == static_cast<std::uint32_t>(vk::PipelineCacheHeaderVersion::eOne) &&
           header.vendorId == properties.vendorID && header.deviceId == properties.deviceID &&
           std::ranges::equal(header.uuid, properties.pipelineCacheUUID);
}

std::vector<std::byte> readPipelineCacheFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        return {};
    }
    std::vector<char> content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    if (file.bad()) {
        return {};
    }
    std::vector<std::byte> bytes(content.size());
    std::memcpy(bytes.data(), content.data(), content.size());
    return bytes;
}

void writePipelineCacheFile(const std::filesystem::path& path, std::span<const std::uint8_t> data) {
    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            throw std::runtime_error{
                std::format("Pipeline cache '{}': cannot create its directory: {}", path.string(), error.message())};
        }
    }
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::vector<char> content(data.size());
        std::memcpy(content.data(), data.data(), data.size());
        std::ofstream file{temporary, std::ios::binary | std::ios::trunc};
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!file) {
            throw std::runtime_error{std::format("Pipeline cache '{}': cannot write it", temporary.string())};
        }
    }
    std::filesystem::rename(temporary, path, error);
    if (error) {
        const std::string reason = error.message();
        std::filesystem::remove(temporary, error);
        throw std::runtime_error{std::format("Pipeline cache '{}': cannot replace it: {}", path.string(), reason)};
    }
}

}  // namespace rtype::render::vulkan::pipeline

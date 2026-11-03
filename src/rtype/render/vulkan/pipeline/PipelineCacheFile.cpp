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
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "core/hash/NameHash.hpp"

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

/// @brief What the engine writes before the Vulkan data in a pipeline cache file, so a damaged file is never handed to
/// the driver: vkCreatePipelineCache must only get data vkGetPipelineCacheData gave
/// (VUID-VkPipelineCacheCreateInfo-initialDataSize-00769), and some drivers crash on damaged data. 24 bytes.
struct FileHeader {
    std::array<char, 4> magic{'R', 'T', 'P', 'C'};  ///< Says the file is an engine pipeline cache.
    std::uint32_t format = 1;                       ///< Version of this header.
    std::uint64_t size = 0;                         ///< Bytes of the Vulkan data after it.
    std::uint64_t checksum = 0;                     ///< FNV-1a 64-bit hash of the Vulkan data.
};
static_assert(sizeof(FileHeader) == 24, "FileHeader is written as is: no padding");

/// @return The FNV-1a 64-bit hash of @p data.
std::uint64_t checksumOf(std::span<const std::byte> data) noexcept {
    std::uint64_t hash = core::hash::kNameHashOffsetBasis;
    for (const std::byte byte : data) {
        hash ^= std::to_integer<std::uint8_t>(byte);
        hash *= core::hash::kNameHashPrime;
    }
    return hash;
}

/// @return A path next to @p path that no other write uses, even by another renderer saving the same cache at the same
/// time: its name, a random 64-bit suffix, then `.tmp`.
/// @throws std::system_error If the system has no source of random numbers.
std::filesystem::path makeTemporaryPath(const std::filesystem::path& path) {
    std::random_device device;
    const std::uint64_t suffix = (std::uint64_t{device()} << 32U) | std::uint64_t{device()};
    std::filesystem::path temporary = path;
    temporary += std::format(".{:016x}.tmp", suffix);
    return temporary;
}

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
    FileHeader header;
    if (content.size() < sizeof(FileHeader)) {
        return {};
    }
    std::memcpy(&header, content.data(), sizeof(FileHeader));
    if (header.magic != FileHeader{}.magic || header.format != FileHeader{}.format ||
        header.size != content.size() - sizeof(FileHeader)) {
        return {};
    }
    std::vector<std::byte> bytes(header.size);
    std::memcpy(bytes.data(), std::span{content}.subspan(sizeof(FileHeader)).data(), bytes.size());
    if (checksumOf(bytes) != header.checksum) {
        return {};
    }
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
    const std::filesystem::path temporary = makeTemporaryPath(path);
    {
        const FileHeader header{.size = data.size(), .checksum = checksumOf(std::as_bytes(data))};
        std::vector<char> content(sizeof(FileHeader) + data.size());
        std::memcpy(content.data(), &header, sizeof(FileHeader));
        std::memcpy(std::span{content}.subspan(sizeof(FileHeader)).data(), data.data(), data.size());
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

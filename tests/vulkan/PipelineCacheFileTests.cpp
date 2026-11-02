/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineCacheFileTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/pipeline/PipelineCacheFile.hpp"

using rtype::render::vulkan::pipeline::isCompatiblePipelineCache;
using rtype::render::vulkan::pipeline::readPipelineCacheFile;
using rtype::render::vulkan::pipeline::writePipelineCacheFile;

namespace {
/// @return The properties of a made-up device: vendor 0x10DE, device 0x2484, a UUID of 1, 2, 3...
vk::PhysicalDeviceProperties makeProperties() {
    vk::PhysicalDeviceProperties properties;
    properties.vendorID = 0x10DE;
    properties.deviceID = 0x2484;
    for (std::uint8_t i = 0; i < VK_UUID_SIZE; ++i) {
        properties.pipelineCacheUUID.at(i) = static_cast<std::uint8_t>(i + 1);
    }
    return properties;
}

/// @return A pipeline cache header for @p properties, then @p extra bytes of data.
std::vector<std::byte> makeCache(const vk::PhysicalDeviceProperties& properties, std::size_t extra = 8) {
    const std::array<std::uint32_t, 4> fields{32, static_cast<std::uint32_t>(vk::PipelineCacheHeaderVersion::eOne),
                                              properties.vendorID, properties.deviceID};
    std::vector<std::byte> data(32 + extra, std::byte{0x5A});
    std::memcpy(data.data(), fields.data(), sizeof(fields));
    std::memcpy(data.data() + sizeof(fields), properties.pipelineCacheUUID.data(), VK_UUID_SIZE);
    return data;
}

/// @return A path in a fresh temporary directory, removed when the test ends.
class TemporaryDirectory {
  public:
    TemporaryDirectory() : _path{std::filesystem::temp_directory_path() / "rtype_pipeline_cache_tests"} {
        std::filesystem::remove_all(_path);
    }
    ~TemporaryDirectory() { std::filesystem::remove_all(_path); }
    TemporaryDirectory(const TemporaryDirectory& other) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory& other) = delete;
    TemporaryDirectory(TemporaryDirectory&& other) = delete;
    TemporaryDirectory& operator=(TemporaryDirectory&& other) = delete;

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< Removed with everything in it.
};

/// @return @p bytes as the uint8_t that PipelineCache::getData() gives.
std::vector<std::uint8_t> toData(std::span<const std::byte> bytes) {
    std::vector<std::uint8_t> data(bytes.size());
    std::memcpy(data.data(), bytes.data(), bytes.size());
    return data;
}
}  // namespace

TEST(PipelineCacheFile, AcceptsACacheOfTheSameDevice) {
    const vk::PhysicalDeviceProperties properties = makeProperties();
    EXPECT_TRUE(isCompatiblePipelineCache(makeCache(properties), properties));
}

TEST(PipelineCacheFile, RejectsACacheOfAnotherDeviceOrDriver) {
    const vk::PhysicalDeviceProperties properties = makeProperties();
    vk::PhysicalDeviceProperties otherVendor = properties;
    otherVendor.vendorID = 0x1002;
    vk::PhysicalDeviceProperties otherDevice = properties;
    otherDevice.deviceID = 0x1234;
    vk::PhysicalDeviceProperties otherDriver = properties;
    otherDriver.pipelineCacheUUID.at(0) = 0xFF;
    EXPECT_FALSE(isCompatiblePipelineCache(makeCache(otherVendor), properties));
    EXPECT_FALSE(isCompatiblePipelineCache(makeCache(otherDevice), properties));
    EXPECT_FALSE(isCompatiblePipelineCache(makeCache(otherDriver), properties));
}

TEST(PipelineCacheFile, RejectsTruncatedOrCorruptedData) {
    const vk::PhysicalDeviceProperties properties = makeProperties();
    EXPECT_FALSE(isCompatiblePipelineCache({}, properties));
    std::vector<std::byte> cache = makeCache(properties);
    EXPECT_FALSE(isCompatiblePipelineCache(std::span{cache}.first(20), properties));
    cache.at(4) = std::byte{7};  // headerVersion
    EXPECT_FALSE(isCompatiblePipelineCache(cache, properties));
    cache = makeCache(properties, 0);
    cache.at(0) = std::byte{200};  // headerSize past the end of the data
    EXPECT_FALSE(isCompatiblePipelineCache(cache, properties));
    cache = makeCache(properties, 8);
    cache.at(0) = std::byte{36};  // within the data, but a version-one header is exactly 32 bytes
    EXPECT_FALSE(isCompatiblePipelineCache(cache, properties));
}

TEST(PipelineCacheFile, ReadsNothingForAMissingFile) {
    const TemporaryDirectory directory;
    EXPECT_TRUE(readPipelineCacheFile(directory.getPath() / "missing.bin").empty());
}

TEST(PipelineCacheFile, WritesThenReadsTheSameBytes) {
    const TemporaryDirectory directory;
    const std::filesystem::path path = directory.getPath() / "nested" / "pipeline_cache.bin";
    const std::vector<std::byte> cache = makeCache(makeProperties());
    writePipelineCacheFile(path, toData(cache));
    EXPECT_EQ(readPipelineCacheFile(path), cache);
    EXPECT_FALSE(std::filesystem::exists(path.string() + ".tmp"));
}

TEST(PipelineCacheFile, WritingReplacesThePreviousFile) {
    const TemporaryDirectory directory;
    const std::filesystem::path path = directory.getPath() / "pipeline_cache.bin";
    writePipelineCacheFile(path, toData(makeCache(makeProperties(), 64)));
    const std::vector<std::byte> smaller = makeCache(makeProperties(), 2);
    writePipelineCacheFile(path, toData(smaller));
    EXPECT_EQ(readPipelineCacheFile(path), smaller);
}

TEST(PipelineCacheFile, ReadsNothingFromADamagedFile) {
    const TemporaryDirectory directory;
    const std::filesystem::path path = directory.getPath() / "pipeline_cache.bin";
    writePipelineCacheFile(path, toData(makeCache(makeProperties())));
    const auto size = std::filesystem::file_size(path);
    {
        std::fstream file{path, std::ios::binary | std::ios::in | std::ios::out};
        file.seekp(static_cast<std::streamoff>(size - 1));
        file.put('\x7F');  // one byte of the Vulkan data changed: the checksum no longer matches
    }
    EXPECT_TRUE(readPipelineCacheFile(path).empty());
    std::filesystem::resize_file(path, size - 4);  // truncated: the size no longer matches
    EXPECT_TRUE(readPipelineCacheFile(path).empty());
}

TEST(PipelineCacheFile, ReadsNothingFromAFileWithoutItsHeader) {
    const TemporaryDirectory directory;
    std::filesystem::create_directories(directory.getPath());
    const std::filesystem::path path = directory.getPath() / "raw.bin";
    const std::vector<std::byte> cache = makeCache(makeProperties());
    {
        std::ofstream file{path, std::ios::binary};
        std::vector<char> content(cache.size());
        std::memcpy(content.data(), cache.data(), cache.size());
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
    EXPECT_TRUE(readPipelineCacheFile(path).empty());
}

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpecializationBuilder
*/

#include "SpecializationBuilder.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::pipeline {

SpecializationBuilder& SpecializationBuilder::add(std::uint32_t constantId, bool value) {
    const vk::Bool32 converted = value ? vk::True : vk::False;
    append(constantId, &converted, sizeof(converted));
    return *this;
}

SpecializationBuilder& SpecializationBuilder::add(std::uint32_t constantId, std::int32_t value) {
    append(constantId, &value, sizeof(value));
    return *this;
}

SpecializationBuilder& SpecializationBuilder::add(std::uint32_t constantId, std::uint32_t value) {
    append(constantId, &value, sizeof(value));
    return *this;
}

SpecializationBuilder& SpecializationBuilder::add(std::uint32_t constantId, float value) {
    append(constantId, &value, sizeof(value));
    return *this;
}

vk::SpecializationInfo SpecializationBuilder::build() const noexcept {
    return vk::SpecializationInfo{}.setMapEntries(_entries).setDataSize(_data.size()).setPData(_data.data());
}

void SpecializationBuilder::append(std::uint32_t constantId, const void* value, std::size_t size) {
    if (std::ranges::any_of(_entries, [constantId](const vk::SpecializationMapEntry& entry) {
            return entry.constantID == constantId;
        })) {
        throw std::invalid_argument(std::format("Specialization constant {} is set twice", constantId));
    }
    const std::size_t offset = _data.size();
    _entries.emplace_back(constantId, static_cast<std::uint32_t>(offset), size);
    _data.resize(offset + size);
    std::memcpy(&_data.at(offset), value, size);
}

}  // namespace rtype::render::vulkan::pipeline

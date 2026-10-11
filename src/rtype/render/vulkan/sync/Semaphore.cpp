/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Semaphore
*/

#include "Semaphore.hpp"

#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::sync {

Semaphore::Semaphore(const core::Device& device) : _semaphore{device.getDevice(), vk::SemaphoreCreateInfo{}} {}

}  // namespace rtype::render::vulkan::sync

/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DeletionQueue
*/

#include <cstddef>
#include <memory>
#include <utility>

namespace rtype::render::vulkan::core {

template <class Resource>
void DeletionQueue::defer(std::size_t frameIndex, Resource resource) {
    bucketOf(frameIndex).push_back(std::make_unique<ResourceEntry<Resource>>(std::move(resource)));
}

}  // namespace rtype::render::vulkan::core

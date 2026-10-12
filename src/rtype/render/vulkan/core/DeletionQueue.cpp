/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DeletionQueue
*/

#include "DeletionQueue.hpp"

#include <cstddef>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace rtype::render::vulkan::core {

DeletionQueue::DeletionQueue(std::size_t framesInFlight) {
    if (framesInFlight == 0) {
        throw std::invalid_argument("DeletionQueue needs at least one frame in flight");
    }
    _frames.resize(framesInFlight);
}

DeletionQueue::~DeletionQueue() { flushAll(); }

std::size_t DeletionQueue::getPendingCount(std::size_t frameIndex) const {
    checkFrameIndex(frameIndex);
    return _frames.at(frameIndex).size();
}

void DeletionQueue::deferCall(std::size_t frameIndex, std::function<void()> function) {
    bucketOf(frameIndex).push_back(std::make_unique<CallEntry>(std::move(function)));
}

void DeletionQueue::flush(std::size_t frameIndex) { clear(bucketOf(frameIndex)); }

void DeletionQueue::flushAll() noexcept {
    for (Bucket& bucket : _frames) {
        clear(bucket);
    }
}

void DeletionQueue::checkFrameIndex(std::size_t frameIndex) const {
    if (frameIndex >= _frames.size()) {
        throw std::out_of_range(
            std::format("DeletionQueue: frame {} out of range ({} frames in flight)", frameIndex, _frames.size()));
    }
}

DeletionQueue::Bucket& DeletionQueue::bucketOf(std::size_t frameIndex) {
    checkFrameIndex(frameIndex);
    return _frames.at(frameIndex);
}

DeletionQueue::CallEntry::~CallEntry() {
    try {
        _function();
    } catch (const std::exception& error) {
        std::cerr << "[DeletionQueue] A deferred call threw: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "[DeletionQueue] A deferred call threw an unknown exception\n";
    }
}

void DeletionQueue::clear(Bucket& bucket) noexcept {
    while (!bucket.empty()) {
        bucket.pop_back();
    }
}

}  // namespace rtype::render::vulkan::core

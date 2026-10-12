/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DeletionQueue
*/

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {

/// @brief Keeps resources alive until the GPU can no longer use them, one bucket per frame in flight.
///
/// @details A resource recorded in frame N's command buffers must not be destroyed before frame N's fence signals.
/// Instead of destroying it, defer() moves it into frame N's bucket; once the renderer has waited on that fence, it
/// calls flush(N), which destroys the bucket's content in reverse order of deferral. Any move-only RAII object fits
/// (vk::raii::*, vma::raii::Buffer...); deferCall() covers handles without one.
///
/// @warning Declare the DeletionQueue after the objects its resources depend on (Device, Allocator), so it is
/// destroyed, and flushes everything, before them, and only after vkDeviceWaitIdle.
class RTYPE_RENDER_VULKAN_API DeletionQueue {
  public:
    /// @param framesInFlight Number of buckets, one per frame in flight. Must not be 0.
    /// @throws std::invalid_argument If @p framesInFlight is 0.
    explicit DeletionQueue(std::size_t framesInFlight);
    /// @brief Destroys every pending resource (flushAll()): the GPU must be idle.
    ~DeletionQueue();
    DeletionQueue(const DeletionQueue&) = delete;
    DeletionQueue& operator=(const DeletionQueue&) = delete;
    DeletionQueue(DeletionQueue&&) = delete;
    DeletionQueue& operator=(DeletionQueue&&) = delete;

    /// @return The number of buckets, one per frame in flight.
    [[nodiscard]] std::size_t getFrameCount() const noexcept { return _frames.size(); }

    /// @return The number of resources and calls waiting in @p frameIndex's bucket.
    /// @throws std::out_of_range If @p frameIndex is not below getFrameCount().
    [[nodiscard]] std::size_t getPendingCount(std::size_t frameIndex) const;

    /// @brief Takes ownership of @p resource until flush(@p frameIndex).
    /// @param frameIndex The frame whose command buffers may still use the resource.
    /// @param resource Destroyed on flush. Taken by value: move RAII objects in (std::move), they cannot be copied.
    /// @throws std::out_of_range If @p frameIndex is not below getFrameCount().
    template <class Resource>
    void defer(std::size_t frameIndex, Resource resource);

    /// @brief Calls @p function on flush(@p frameIndex), for handles that have no RAII owner.
    /// @param function Must not throw: it runs from destructors.
    /// @throws std::out_of_range If @p frameIndex is not below getFrameCount().
    void deferCall(std::size_t frameIndex, std::function<void()> function);

    /// @brief Destroys the content of @p frameIndex's bucket, last deferred first. Call it once the frame's fence has
    /// signaled.
    /// @throws std::out_of_range If @p frameIndex is not below getFrameCount().
    void flush(std::size_t frameIndex);

    /// @brief Flushes every bucket. Only when the GPU is idle (after vkDeviceWaitIdle), e.g. at shutdown.
    void flushAll() noexcept;

  private:
    /// @brief Type-erased owner of one deferred resource or call: destroying it destroys the resource.
    class Entry {
      public:
        Entry() = default;
        virtual ~Entry() = default;
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
        Entry(Entry&&) = delete;
        Entry& operator=(Entry&&) = delete;
    };

    /// @brief Owns a deferred resource of type Resource.
    template <class Resource>
    class ResourceEntry final : public Entry {
      public:
        explicit ResourceEntry(Resource resource) : _resource{std::move(resource)} {}
        ~ResourceEntry() override = default;
        ResourceEntry(const ResourceEntry&) = delete;
        ResourceEntry& operator=(const ResourceEntry&) = delete;
        ResourceEntry(ResourceEntry&&) = delete;
        ResourceEntry& operator=(ResourceEntry&&) = delete;

      private:
        Resource _resource;  ///< The resource, destroyed with the entry.
    };

    /// @brief Calls a deferred function when destroyed.
    class CallEntry final : public Entry {
      public:
        explicit CallEntry(std::function<void()> function) : _function{std::move(function)} {}
        /// @brief Calls the function; an exception it throws is reported, not propagated (destructor).
        ~CallEntry() override;
        CallEntry(const CallEntry&) = delete;
        CallEntry& operator=(const CallEntry&) = delete;
        CallEntry(CallEntry&&) = delete;
        CallEntry& operator=(CallEntry&&) = delete;

      private:
        std::function<void()> _function;  ///< Called on destruction.
    };

    using Bucket = std::vector<std::unique_ptr<Entry>>;

    /// @throws std::out_of_range If @p frameIndex is not below getFrameCount().
    void checkFrameIndex(std::size_t frameIndex) const;

    /// @return The bucket of @p frameIndex.
    /// @throws std::out_of_range If @p frameIndex is not below getFrameCount().
    Bucket& bucketOf(std::size_t frameIndex);

    /// @brief Destroys the entries of @p bucket, last deferred first.
    static void clear(Bucket& bucket) noexcept;

    std::vector<Bucket> _frames;  ///< One bucket per frame in flight.
};
}  // namespace rtype::render::vulkan::core

#include "DeletionQueue.tpp"

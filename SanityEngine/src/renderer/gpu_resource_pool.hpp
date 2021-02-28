#pragma once

#include "core/types.hpp"
#include "renderer/handles.hpp"
#include "renderer/rhi/per_frame_buffer.hpp"
#include "renderer/rhi/resources.hpp"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    class Renderer;

    class HandlePool {
    public:
        [[nodiscard]] Uint32 allocate_handle();

        void free_handle(Uint32 handle);

    private:
        Uint32 next_handle{0};

        Rx::Vector<Uint32> available_handles;
    };

    template <typename ResourceType>
    class GpuResourcePool {
    public:
        explicit GpuResourcePool(Uint32 capacity_in, BufferRing storage_in);

        GpuResourcePool(const GpuResourcePool& other) = delete;
        GpuResourcePool& operator=(const GpuResourcePool& other) = delete;

        GpuResourcePool(GpuResourcePool&& old) noexcept = default;
        GpuResourcePool& operator=(GpuResourcePool&& old) noexcept = default;

        ~GpuResourcePool() = default;

        [[nodiscard]] GpuResourceHandle<ResourceType> allocate();

        void free(const GpuResourceHandle<ResourceType>& handle);

        void commit_frame(Uint32 frame_idx);

    private:
        HandlePool handles;

        Rx::Vector<ResourceType> host_storage;

        BufferRing device_storage;
    };

    template <typename ResourceType>
    GpuResourcePool<ResourceType>::GpuResourcePool(Uint32 capacity_in, BufferRing storage_in)
        : device_storage{Rx::Utility::move(storage_in)} {
        host_storage.reserve(capacity_in);
    }
} // namespace sanity::engine::renderer

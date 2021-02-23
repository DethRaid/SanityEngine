#pragma once

#include "core/types.hpp"
#include "renderer/hlsl/constants.hpp"
#include "rx/core/abort.h"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    template <typename ResourceType>
    struct GpuResourceHandle {
        Uint32 index{INVALID_RESOURCE_HANDLE};

        GpuResourceHandle() = default;

        explicit GpuResourceHandle(Uint32 index_in);

        GpuResourceHandle(const GpuResourceHandle<ResourceType>& other) = default;
        GpuResourceHandle& operator=(const GpuResourceHandle<ResourceType>& other) = default;

        GpuResourceHandle(GpuResourceHandle<ResourceType>&& old) noexcept = default;
        GpuResourceHandle& operator=(GpuResourceHandle<ResourceType>&& old) noexcept = default;

        ~GpuResourceHandle() = default;

        [[nodiscard]] bool operator==(const GpuResourceHandle& other) const = default;

        [[nodiscard]] bool is_valid() const;
    };

    template <typename ResourceType>
    GpuResourceHandle<ResourceType>::GpuResourceHandle(const Uint32 index_in) : index{index_in} {}

    template <typename ResourceType>
    bool GpuResourceHandle<ResourceType>::is_valid() const {
        return index != INVALID_RESOURCE_HANDLE;
    }
} // namespace sanity::engine::renderer

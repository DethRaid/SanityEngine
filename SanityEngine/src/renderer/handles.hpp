#pragma once

#include "core/types.hpp"
#include "rx/core/abort.h"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    static constexpr auto INVALID_RESOURCE_HANDLE = 0xFFFFFFFF;
	
    template <typename ResourceType>
    struct GpuResourceHandle {    	
        Uint32 index{INVALID_RESOURCE_HANDLE};

        Rx::Vector<ResourceType>* storage{nullptr};

        GpuResourceHandle() = default;

        GpuResourceHandle(Uint32 index_in, Rx::Vector<ResourceType>* storage_in);

        GpuResourceHandle(const GpuResourceHandle<ResourceType>& other) = default;
        GpuResourceHandle& operator=(const GpuResourceHandle<ResourceType>& other) = default;

        GpuResourceHandle(GpuResourceHandle<ResourceType>&& old) noexcept = default;
        GpuResourceHandle& operator=(GpuResourceHandle<ResourceType>&& old) noexcept = default;

        ~GpuResourceHandle() = default;

        [[nodiscard]] bool operator==(const GpuResourceHandle& other) const = default;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] ResourceType* operator->() const;

        [[nodiscard]] ResourceType& operator*() const;
    };

    template <typename ResourceType>
    GpuResourceHandle<ResourceType>::GpuResourceHandle(const Uint32 index_in, Rx::Vector<ResourceType>* storage_in)
        : index{index_in}, storage{storage_in} {}

    template <typename ResourceType>
    bool GpuResourceHandle<ResourceType>::is_valid() const {
        return index != INVALID_RESOURCE_HANDLE && storage != nullptr && index < storage->size();
    }

    template <typename ResourceType>
    ResourceType* GpuResourceHandle<ResourceType>::operator->() const {
        if(!is_valid()) {
            return nullptr;
        }

        return &(*storage)[index];
    }

    template <typename ResourceType>
    ResourceType& GpuResourceHandle<ResourceType>::operator*() const {
        if(!is_valid()) {
            Rx::abort("Invalid handle");
        }

        return (*storage)[index];
    }
} // namespace sanity::engine::renderer

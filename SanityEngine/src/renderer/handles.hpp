#pragma once

#include "core/types.hpp"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    template <typename ResourceType>
    struct GpuResourceHandle {
        Uint32 index{0xFFFFFFFF};

        Rx::Vector<ResourceType>* storage{nullptr};

        [[nodiscard]] bool operator==(const GpuResourceHandle& other) const = default;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] ResourceType* operator->() const;
    };

    template <typename ResourceType>
    bool GpuResourceHandle<ResourceType>::is_valid() const {
        return index != 0xFFFFFFFF && storage != nullptr;
    }

    template <typename ResourceType>
    ResourceType* GpuResourceHandle<ResourceType>::operator->() const {
        if(!is_valid()) {
            return nullptr;
        }

        return &(*storage)[index];
    }

} // namespace sanity::engine::renderer

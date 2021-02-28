#pragma once

#include "core/types.hpp"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    class Renderer;

    /**
     * @brief A ring buffer of an arbitrary type of resource
     *
     * This class must be overridden for each resource type. The subclass must initialize the vector `resources`. Whether subclasses create
     * the resources, receive them as a parameter, whatever, doesn't matter as long as you initialize `resources`
     *
     * Implementor's note: The length of `resources` is the length of the ring buffer. Subclasses should take care that the length is
     * correct
     *
     * @tparam ResourceType Type of resource to store in the ring
     */
    template <typename ResourceType>
    class ResourceRing {
    public:
        ResourceRing(const ResourceRing& other) = default;
        ResourceRing& operator=(const ResourceRing& other) = default;

        ResourceRing(ResourceRing&& old) noexcept = default;
        ResourceRing& operator=(ResourceRing&& old) noexcept = default;

        void advance_frame();

        [[nodiscard]] const ResourceType& get_active_resource() const;

        virtual ~ResourceRing() = default;

    protected:
        Rx::Vector<ResourceType> resources;
        Uint32 active_idx{0};

        ResourceRing() = default;
    };

    template <typename ResourceType>
    void ResourceRing<ResourceType>::advance_frame() {
        active_idx++;
        if(active_idx >= resources.size()) {
            active_idx = 0;
        }
    }

    template <typename ResourceType>
    const ResourceType& ResourceRing<ResourceType>::get_active_resource() const {
        return resources[active_idx];
    }
} // namespace sanity::engine::renderer

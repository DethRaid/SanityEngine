#pragma once

#include "core/types.hpp"
#include "renderer/render_pass.hpp"
#include "rx/core/ptr.h"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    template <typename RenderPassType>
    class RenderpassHandle {
    public:
        /**
         * @brief Makes a handle to the last render pass in the container
         */
        [[nodiscard]] static RenderpassHandle<RenderPassType> make_from_last_element(Rx::Vector<Rx::Ptr<RenderPass>>& container);

        RenderpassHandle() = default;

        explicit RenderpassHandle(Rx::Vector<Rx::Ptr<RenderPass>>& container_in, Size index_in);

        RenderPassType* operator*() const;

        RenderPassType* operator->() const;

        [[nodiscard]] Size get_index() const;

    private:
        Rx::Vector<Rx::Ptr<RenderPass>>* vector{nullptr};

        Size index{};

        [[nodiscard]] RenderPass* get_renderpass() const;
    };

    template <typename RenderPassType>
    RenderpassHandle<RenderPassType> RenderpassHandle<RenderPassType>::make_from_last_element(Rx::Vector<Rx::Ptr<RenderPass>>& container) {
        return RenderpassHandle<RenderPassType>{container, container.size() - 1};
    }

    template <typename RenderPassType>
    RenderpassHandle<RenderPassType>::RenderpassHandle(Rx::Vector<Rx::Ptr<RenderPass>>& container_in, const Size index_in)
        : vector{&container_in}, index{index_in} {}

    template <typename RenderPassType>
    RenderPassType* RenderpassHandle<RenderPassType>::operator*() const {
        auto* renderpass = get_renderpass();
        return static_cast<RenderPassType*>(renderpass);
    }

    template <typename RenderPassType>
    RenderPassType* RenderpassHandle<RenderPassType>::operator->() const {
        auto* renderpass = get_renderpass();
        return static_cast<RenderPassType*>(renderpass);
    }

    template <typename RenderPassType>
    Size RenderpassHandle<RenderPassType>::get_index() const {
        return index;
    }

    template <typename RenderPassType>
    RenderPass* RenderpassHandle<RenderPassType>::get_renderpass() const {
        return (*vector)[index].get();
    }
} // namespace sanity::engine::renderer

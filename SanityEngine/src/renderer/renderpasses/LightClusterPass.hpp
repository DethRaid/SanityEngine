#pragma once

#include "renderer/renderpass.hpp"

namespace renderer {
    /*!
     * \brief Sorts lights into frustum-aligned clusters
     */
    class LightClusterPass : public RenderPass {
    public:
        explicit LightClusterPass() = default;

        ~LightClusterPass() override = default;
    };
} // namespace renderer

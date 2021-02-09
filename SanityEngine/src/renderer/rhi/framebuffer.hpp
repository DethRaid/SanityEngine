#pragma once

#include "glm/vec4.hpp"
#include "resources.hpp"
#include "rx/core/optional.h"

namespace sanity::engine::renderer {
    enum class RenderTargetBeginningAccessType {
        /*!
         * \brief Load the data that was previously rendered to this render target
         */
        Preserve,

        /*!
         * \brief Clear the render target to a constant color
         */
        Clear,

        /*!
         * Don't care what's in the render target
         */
        Discard
    };

    struct RenderTargetBeginningAccess {
        /*!
         * \brief What to do with the render target
         */
        RenderTargetBeginningAccessType type{RenderTargetBeginningAccessType::Preserve};

        /*!
         * Color to clear a render target to. Only relevant if `type` is `RenderTargetBeginningAccessType::Clear
         */
        glm::vec4 clear_color{};

        TextureFormat format{};
    };

    enum class RenderTargetEndingAccessType {
        /*!
         * \brief Preserve the contents of the render target for future access
         */
        Preserve,

        /*!
         * \brief Resolve the contents of the render target, such as resolving MSAA
         */
        Resolve,

        /*!
         * \brief Don't care what happens to the render target contents
         */
        Discard
    };

    /*!
     * \brief How to resolve a render target
     */
    struct RenderTargetResolveParameters {
        /*!
         * \brief Image to resolve to
         */
        Texture* resolve_target;

        /*!
         * \brief Whether to preserve the image you're resolving
         */
        bool preserve_resolve_source{false};
    };

    struct RenderTargetEndingAccess {
        /*!
         * \brief What to do with the render target
         */
        RenderTargetEndingAccessType type{RenderTargetEndingAccessType::Preserve};

        /*!
         * \brief How to resolve the render target. Only relevant if `type` is `RenderTargetEndingAccessType::Resolve`
         */
        RenderTargetResolveParameters resolve_params{};
    };

    struct RenderTargetAccess {
        RenderTargetBeginningAccess begin;
        RenderTargetEndingAccess end;
    };
} // namespace renderer

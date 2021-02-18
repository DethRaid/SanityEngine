#pragma once

#include "glm/fwd.hpp"
#include "renderer/debugging/pix.hpp"
#include "renderer/handles.hpp"
#include "renderer/renderpass.hpp"
#include "renderer/rhi/descriptor_allocator.hpp"
#include "renderer/rhi/framebuffer.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace sanity::engine::renderer {
    class ObjectsPass;
    class Renderer;

    class DenoiserPass final : public RenderPass {
    public:
        /*!
         * \brief Constructs a new denoiser pass to denoise some stuff
         *
         * \param renderer_in The renderer which will be executing this pass
         * \param render_resolution The resolution to render at. May or may not equal the final resolution
         * \param forward_pass the pass which this denoise pass will denoise the output of
         */
        explicit DenoiserPass(Renderer& renderer_in, const glm::uvec2& render_resolution, const ObjectsPass& forward_pass);

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

        [[nodiscard]] TextureHandle get_output_texture() const;

    private:
        Uint64 denoiser_pass_color{PIX_COLOR(91, 133, 170)};
        Renderer* renderer;

        Rx::Ptr<RenderPipelineState> denoising_pipeline;

        /*!
         * \brief Handle to the texture that holds the accumulated scene
         */
        TextureHandle accumulation_target_handle;

        /*!
         * \brief Handle to the texture that holds the final denoised image
         */
        TextureHandle denoised_color_target_handle;

        /*!
         * \brief RTV for the final denoised image
         */
        DescriptorRange denoised_rtv_handle;

        BufferHandle denoiser_material_buffer_handle;

        void create_textures_and_framebuffer(const glm::uvec2& render_resolution);

        void create_material(const ObjectsPass& forward_pass);
    };
} // namespace renderer

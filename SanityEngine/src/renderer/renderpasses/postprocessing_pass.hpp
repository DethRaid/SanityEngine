#pragma once

#include "renderer/debugging/pix.hpp"
#include "renderer/renderpass.hpp"
#include "renderer/rhi/render_device.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace sanity::engine::renderer {
    class Renderer;
    class DenoiserPass;

    class PostprocessingPass final : public RenderPass {
    public:
        explicit PostprocessingPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass);

        ~PostprocessingPass() override = default;

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

    	void set_output_texture(TextureHandle new_output_texture_handle);

    	[[nodiscard]] TextureHandle get_output_texture() const;

    private:
        Renderer* renderer{nullptr};

        Uint64 postprocessing_pass_color{PIX_COLOR(91, 133, 170)};

        Rx::Ptr<RenderPipelineState> postprocessing_pipeline;

        Rx::Ptr<Buffer> postprocessing_materials_buffer;
    	
        TextureHandle output_texture_handle{0};
    	
        DescriptorRange output_rtv_handle{};
    };
} // namespace renderer

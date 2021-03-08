#pragma once

#include "renderer/debugging/pix.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/rhi/render_backend.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace sanity::engine::renderer {
    class Renderer;
    class CompositingPass;

    class PostprocessingPass final : public RenderPass {
    public:
        explicit PostprocessingPass(Renderer& renderer_in, const CompositingPass& scene_compositing_pass);

        ~PostprocessingPass() override = default;

        void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

    	void set_output_texture(TextureHandle new_output_texture_handle);

    	[[nodiscard]] TextureHandle get_output_texture() const;

    private:
        Renderer* renderer{nullptr};

        Uint64 postprocessing_pass_color{PIX_COLOR(91, 133, 170)};

        Rx::Ptr<RenderPipelineState> postprocessing_pipeline;

        BufferHandle postprocessing_material_buffer_handle;
    	
        TextureHandle output_texture_handle{};
    	
        DescriptorRange output_rtv_handle{};
    };
} // namespace renderer

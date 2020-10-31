#include "renderpass.hpp"

namespace renderer {
    void RenderPass::issue_pre_pass_barriers(ID3D12GraphicsCommandList* commands) { 
        used_resources.each_fwd([&](const ResourceUsage& usage) {
                
        });
    }
    
    void RenderPass::issue_post_pass_barriers(ID3D12GraphicsCommandList* commands) {}
} // namespace renderer

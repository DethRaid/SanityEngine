#include "d3d12_render_command_list.hpp"

#include <minitrace.h>

#include "d3d12_framebuffer.hpp"

using rx::utility::move;

namespace render {
    D3D12RenderCommandList::D3D12RenderCommandList(rx::memory::allocator& allocator,
                                                   ComPtr<ID3D12GraphicsCommandList> cmds,
                                                   D3D12RenderDevice& device_in)
        : D3D12ComputeCommandList{allocator, move(cmds), device_in} {
        commands->QueryInterface(commands4.GetAddressOf());
    }

    void D3D12RenderCommandList::set_framebuffer(const Framebuffer& framebuffer) {
        MTR_SCOPE("D3D12RenderCommandList", "set_render_targets");

        const D3D12Framebuffer& d3d12_framebuffer = static_cast<const D3D12Framebuffer&>(framebuffer);

        if(commands4) {
            // TODO: Decide if every framebuffer should be part of a separate renderpass

            RX_ASSERT(d3d12_framebuffer.rtv_handles.size() == d3d12_framebuffer.render_target_descriptions.size(),
                      "Render target descriptions and rtv handles must have the same length");
            RX_ASSERT(static_cast<bool>(d3d12_framebuffer.depth_stencil_desc) == static_cast<bool>(d3d12_framebuffer.dsv_handle),
                      "If a framebuffer has a depth attachment, it must have both a DSV handle and a depth_stencil description");

            if(in_render_pass) {
                commands4->EndRenderPass();
            }

            if(d3d12_framebuffer.depth_stencil_desc) {
                commands4->BeginRenderPass(static_cast<UINT>(d3d12_framebuffer.render_target_descriptions.size()),
                                           d3d12_framebuffer.render_target_descriptions.data(),
                                           &(*d3d12_framebuffer.depth_stencil_desc),
                                           D3D12_RENDER_PASS_FLAG_NONE);
            } else {
                commands4->BeginRenderPass(static_cast<UINT>(d3d12_framebuffer.render_target_descriptions.size()),
                                           d3d12_framebuffer.render_target_descriptions.data(),
                                           nullptr,
                                           D3D12_RENDER_PASS_FLAG_NONE);
            }

            in_render_pass = true;
        }

        if(d3d12_framebuffer.dsv_handle) {
            commands->OMSetRenderTargets(static_cast<UINT>(d3d12_framebuffer.rtv_handles.size()),
                                         d3d12_framebuffer.rtv_handles.data(),
                                         0,
                                         &(*d3d12_framebuffer.dsv_handle));

        } else {
            commands->OMSetRenderTargets(static_cast<UINT>(d3d12_framebuffer.rtv_handles.size()),
                                         d3d12_framebuffer.rtv_handles.data(),
                                         0,
                                         nullptr);
        }
    }
} // namespace render

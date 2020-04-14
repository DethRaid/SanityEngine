#include "d3d12_render_command_list.hpp"

#include "minitrace.h"
using rx::utility::move;

namespace render {
    D3D12RenderCommandList::D3D12RenderCommandList(rx::memory::allocator& allocator,
                                                   ComPtr<ID3D12GraphicsCommandList> cmds,
                                                   D3D12RenderDevice& device_in)
        : D3D12ComputeCommandList{allocator, move(cmds), device_in} {
        commands->QueryInterface(commands4.GetAddressOf());
    }

    void D3D12RenderCommandList::set_render_targets(const rx::vector<Image*>& color_targets, Image* depth_target) {
        MTR_SCOPE("D3D12RenderCommandList", "set_render_targets");

            RX_ASSERT(color_targets.size() >= 8, "May only use eight color render targets at a single time");

        rx::array<D3D12_CPU_DESCRIPTOR_HANDLE[8]> rtvs;

        for(uint32_t i = 0; i < color_targets.size(); i++) {
            D3D12Image* d3d12_image = static_cast<D3D12Image*>(color_targets[i]);
            rtvs[i] = device->get_rtv_for_image(*d3d12_image);
        }

        if(depth_target != nullptr) {
            D3D12Image* d3d12_depth_target = static_cast<D3D12Image*>(depth_target);
            const auto dsv = device->get_dsv_for_image(*d3d12_depth_target);

            commands->OMSetRenderTargets(static_cast<UINT>(color_targets.size()), rtvs.data(), 0, &dsv);

        } else {
            commands->OMSetRenderTargets(static_cast<UINT>(color_targets.size()), rtvs.data(), 0, nullptr);
        }
    }
} // namespace render

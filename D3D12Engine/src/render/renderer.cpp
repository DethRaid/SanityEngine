#include "renderer.hpp"

#include <rx/console/variable.h>

#include "d3d12/d3d12_render_device.hpp"

namespace render {
    rx::ptr<RenderDevice> make_render_device(rx::memory::allocator& allocator, const RenderBackend backend) {
        switch(backend) {
            case RenderBackend::D3D12:
                return rx::make_ptr<D3D12RenderDevice>(allocator);
        }

        return {};
    }
} // namespace render

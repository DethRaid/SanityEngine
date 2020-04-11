#pragma once

#include <rx/core/ptr.h>

#include "compute_command_list.hpp"
#include "graphics_command_list.hpp"
#include "resource_command_list.hpp"

namespace render {
    enum class RenderBackend {
        D3D12,
    };

    /*
     * \brief A device which can be used to render
     */
    class RenderDevice : rx::concepts::interface {
    public:
        [[nodiscard]] virtual rx::ptr<Buffer> create_buffer(rx::memory::allocator& allocator, const BufferCreateInfo& create_info) = 0;

        [[nodiscard]] virtual rx::ptr<Image> create_image(rx::memory::allocator& allocator, const ImageCreateInfo& create_info) = 0;

        [[nodiscard]] virtual void* map_buffer(const Buffer& buffer) = 0;

        virtual void destroy_buffer(rx::ptr<Buffer> buffer) = 0;

        virtual void destroy_image(rx::ptr<Image> image) = 0;

        [[nodiscard]] virtual rx::ptr<ResourceCommandList> get_resource_command_list() = 0;

        [[nodiscard]] virtual rx::ptr<ComputeCommandList> get_compute_command_list() = 0;

        [[nodiscard]] virtual rx::ptr<GraphicsCommandList> get_graphics_command_list() = 0;

        void virtual submit_command_list(rx::ptr<CommandList> commands) = 0;
    };

    rx::ptr<RenderDevice> make_render_device(rx::memory::allocator& allocator, RenderBackend backend);
} // namespace render

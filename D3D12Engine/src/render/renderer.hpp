#pragma once

#ifdef interface
#undef interface
#endif

#include <rx/core/ptr.h>

#include "compute_command_list.hpp"
#include "render_command_list.hpp"
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
#pragma region Resources
        [[nodiscard]] virtual rx::ptr<Buffer> create_buffer(rx::memory::allocator& allocator, const BufferCreateInfo& create_info) = 0;

        [[nodiscard]] virtual rx::ptr<Image> create_image(rx::memory::allocator& allocator, const ImageCreateInfo& create_info) = 0;

        [[nodiscard]] virtual rx::ptr<Framebuffer> create_framebuffer(const rx::vector<const Image*>& render_targets,
                                                                      const Image* depth_target = nullptr) = 0;

        [[nodiscard]] virtual void* map_buffer(const Buffer& buffer) = 0;

        virtual void destroy_buffer(rx::ptr<Buffer> buffer) = 0;

        virtual void destroy_image(rx::ptr<Image> image) = 0;

        virtual void destroy_framebuffer(rx::ptr<Framebuffer> framebuffer) = 0;
#pragma endregion

#pragma region Pipeline
        [[nodiscard]] virtual rx::ptr<ComputePipelineState> create_compute_pipeline_state() = 0;

        [[nodiscard]] virtual rx::ptr<RenderPipelineState> create_render_pipeline_state() = 0;

        virtual void destroy_compute_pipeline_state(rx::ptr<ComputePipelineState> pipeline_state) = 0;

        virtual void destroy_render_pipeline_state(rx::ptr<RenderPipelineState> pipeline_state) = 0;
#pragma endregion

#pragma region Command lists
        [[nodiscard]] virtual rx::ptr<ResourceCommandList> create_resource_command_list() = 0;

        [[nodiscard]] virtual rx::ptr<ComputeCommandList> create_compute_command_list() = 0;

        [[nodiscard]] virtual rx::ptr<RenderCommandList> create_render_command_list() = 0;

        void virtual submit_command_list(rx::ptr<CommandList> commands) = 0;
#pragma endregion

#pragma region Rendering
        /*!
         * \brief Sets up the render device to render the next frame
         *
         * This probably means waiting on the previous frame to complete on the GPU
         */
        virtual void begin_frame() = 0;
#pragma endregion
    };

    [[nodiscard]] rx::ptr<RenderDevice> make_render_device(rx::memory::allocator& allocator, RenderBackend backend);
} // namespace render

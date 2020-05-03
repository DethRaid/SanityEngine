#pragma once

#include <memory>
#include <vector>

#include "bind_group.hpp"
#include "compute_command_list.hpp"
#include "render_command_list.hpp"
#include "resource_command_list.hpp"

struct Settings;
struct GLFWwindow;

namespace rhi {
    struct RenderPipelineStateCreateInfo;

    enum class RenderBackend {
        D3D12,
    };

    /*
     * \brief A device which can be used to render
     *
     * A render device - and by extension the CPU - may only record commands for a single frame at a time. However, the GPU may be executing
     * one frame for each image in the swapchain. Most of the synchronization concerns should be hidden behind this interface, but be aware
     * that the frame that the GPU may be several frames behind the CPU
     */
    class RenderDevice {
    public:
        virtual ~RenderDevice() = default;

#pragma region Resources
        [[nodiscard]] virtual std::unique_ptr<Buffer> create_buffer(const BufferCreateInfo& create_info) = 0;

        [[nodiscard]] virtual std::unique_ptr<Image> create_image(const ImageCreateInfo& create_info) = 0;

        [[nodiscard]] virtual std::unique_ptr<Framebuffer> create_framebuffer(const std::vector<const Image*>& render_targets,
                                                                              const Image* depth_target = nullptr) = 0;

        [[nodiscard]] virtual Framebuffer* get_backbuffer_framebuffer() = 0;

        [[nodiscard]] virtual void* map_buffer(const Buffer& buffer) = 0;

        virtual void destroy_buffer(std::unique_ptr<Buffer> buffer) = 0;

        virtual void destroy_image(std::unique_ptr<Image> image) = 0;

        virtual void destroy_framebuffer(std::unique_ptr<Framebuffer> framebuffer) = 0;
#pragma endregion

#pragma region Pipeline
        [[nodiscard]] virtual std::unique_ptr<ComputePipelineState> create_compute_pipeline_state(
            const std::vector<uint8_t>& compute_shader) = 0;

        [[nodiscard]] virtual std::unique_ptr<RenderPipelineState> create_render_pipeline_state(
            const RenderPipelineStateCreateInfo& create_info) = 0;

        [[nodiscard]] virtual std::pair<std::unique_ptr<RenderPipelineState>, std::unique_ptr<BindGroupBuilder>>
        create_bespoke_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info) = 0;

        virtual void destroy_compute_pipeline_state(std::unique_ptr<ComputePipelineState> pipeline_state) = 0;

        virtual void destroy_render_pipeline_state(std::unique_ptr<RenderPipelineState> pipeline_state) = 0;
#pragma endregion

#pragma region Command lists
        [[nodiscard]] virtual std::unique_ptr<ResourceCommandList> create_resource_command_list() = 0;

        [[nodiscard]] virtual std::unique_ptr<ComputeCommandList> create_compute_command_list() = 0;

        [[nodiscard]] virtual std::unique_ptr<RenderCommandList> create_render_command_list() = 0;

        void virtual submit_command_list(std::unique_ptr<CommandList> commands) = 0;
#pragma endregion

#pragma region Rendering
        [[nodiscard]] virtual BindGroupBuilder& get_material_bind_group_builder() = 0;

        /*!
         * \brief Sets up the render device to render the next frame
         *
         * This probably means waiting on the previous frame to complete on the GPU
         */
        virtual void begin_frame(uint64_t frame_count) = 0;

        /*!
         * \brief Performs all the actions necessary to end the frame
         *
         * This probably means presenting the backbuffer
         */
        virtual void end_frame() = 0;

        [[nodiscard]] virtual uint32_t get_cur_gpu_frame_idx() = 0;
#pragma endregion
    };

    [[nodiscard]] std::unique_ptr<RenderDevice> make_render_device(RenderBackend backend, GLFWwindow* window, const Settings& settings);
} // namespace rhi

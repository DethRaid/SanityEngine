#pragma once
#include <atomic>
#include <mutex>
#include <queue>

#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <spdlog/logger.h>
#include <wrl/client.h>

#include "../../core/async/mutex.hpp"
#include "../../settings.hpp"
#include "../bind_group.hpp"
#include "../render_device.hpp"
#include "d3d12_bind_group.hpp"
#include "d3d12_command_list.hpp"
#include "d3d12_descriptor_allocator.hpp"
#include "d3d12_framebuffer.hpp"
#include "resources.hpp"

namespace DirectX {
    struct XMINT2;
}

using DirectX::XMINT2;

namespace rhi {
    using Microsoft::WRL::ComPtr;

    class D3D12RenderDevice : public virtual RenderDevice {
    public:
        D3D12RenderDevice(HWND window_handle, const XMINT2& window_size, const Settings& settings_in);

        D3D12RenderDevice(const D3D12RenderDevice& other) = delete;
        D3D12RenderDevice& operator=(const D3D12RenderDevice& other) = delete;

        D3D12RenderDevice(D3D12RenderDevice&& old) noexcept = delete;
        D3D12RenderDevice& operator=(D3D12RenderDevice&& old) noexcept = delete;

        ~D3D12RenderDevice() override;

#pragma region RenderDevice
        [[nodiscard]] std::unique_ptr<Buffer> create_buffer(const BufferCreateInfo& create_info) override;

        [[nodiscard]] std::unique_ptr<Image> create_image(const ImageCreateInfo& create_info) override;

        [[nodiscard]] std::unique_ptr<Framebuffer> create_framebuffer(const std::vector<const Image*>& render_targets,
                                                                      const Image* depth_target) override;

        Framebuffer* get_backbuffer_framebuffer() override;

        void* map_buffer(const Buffer& buffer) override;

        void destroy_buffer(std::unique_ptr<Buffer> buffer) override;

        void destroy_image(std::unique_ptr<Image> image) override;

        void destroy_framebuffer(std::unique_ptr<Framebuffer> framebuffer) override;

        [[nodiscard]] std::unique_ptr<ComputePipelineState> create_compute_pipeline_state(
            const std::vector<uint8_t>& compute_shader) override;

        [[nodiscard]] std::unique_ptr<RenderPipelineState> create_render_pipeline_state(
            const RenderPipelineStateCreateInfo& create_info) override;

        [[nodiscard]] std::pair<std::unique_ptr<RenderPipelineState>, std::unique_ptr<BindGroupBuilder>>
        create_bespoke_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info) override;

        void destroy_compute_pipeline_state(std::unique_ptr<ComputePipelineState> pipeline_state) override;

        void destroy_render_pipeline_state(std::unique_ptr<RenderPipelineState> pipeline_state) override;

        [[nodiscard]] std::unique_ptr<ResourceCommandList> create_resource_command_list() override;

        [[nodiscard]] std::unique_ptr<ComputeCommandList> create_compute_command_list() override;

        [[nodiscard]] std::unique_ptr<RenderCommandList> create_render_command_list() override;

        void submit_command_list(std::unique_ptr<CommandList> commands) override;

        BindGroupBuilder& get_material_bind_group_builder() override;

        void begin_frame(uint64_t frame_count) override;

        void end_frame() override;

        uint32_t get_cur_gpu_frame_idx() override;
#pragma endregion

        [[nodiscard]] bool has_separate_device_memory() const;

        [[nodiscard]] D3D12StagingBuffer get_staging_buffer(uint32_t num_bytes);

        void return_staging_buffer(D3D12StagingBuffer&& buffer);

        [[nodiscard]] ID3D12Device* get_d3d12_device() const;

        [[nodiscard]] UINT get_shader_resource_descriptor_size() const;

    private:
        Settings settings;

        std::shared_ptr<spdlog::logger> logger;

        ComPtr<ID3D12Debug> debug_controller;
        ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred_settings;

        ComPtr<IDXGIFactory4> factory;

        ComPtr<IDXGIAdapter> adapter;

        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12Device1> device1;

        ComPtr<ID3D12InfoQueue> info_queue;

        ComPtr<ID3D12CommandQueue> direct_command_queue;

        ComPtr<ID3D12CommandQueue> async_copy_queue;

        std::vector<ComPtr<ID3D12CommandAllocator>> direct_command_allocators;

        std::vector<ComPtr<ID3D12CommandAllocator>> compute_command_allocators;

        std::vector<ComPtr<ID3D12CommandAllocator>> copy_command_allocators;

        ComPtr<IDXGISwapChain3> swapchain;
        std::vector<ComPtr<ID3D12Resource>> swapchain_images;
        std::vector<D3D12Framebuffer> swapchain_framebuffers;

        HANDLE frame_event;
        ComPtr<ID3D12Fence> frame_fences;
        std::vector<uint32_t> frame_fence_values;

        ComPtr<ID3D12DescriptorHeap> cbv_srv_uav_heap;
        UINT cbv_srv_uav_size{};
        INT next_free_cbv_srv_uav_descriptor{0};

        std::unique_ptr<D3D12DescriptorAllocator> rtv_allocator;

        std::unique_ptr<D3D12DescriptorAllocator> dsv_allocator;

        D3D12MA::Allocator* device_allocator;

        ComPtr<ID3D12RootSignature> standard_root_signature;

        std::vector<D3D12_INPUT_ELEMENT_DESC> standard_graphics_pipeline_input_layout;

        uint64_t staging_buffer_idx{0};
        std::vector<D3D12StagingBuffer> staging_buffers;

        /*!
         * \brief Array of array of staging buffers to free on a frame. index 0 gets freed on the next frame 0, index 1 gets freed on the
         * next frame 1, etc
         */
        std::vector<std::vector<D3D12StagingBuffer>> staging_buffers_to_free;

        /*!
         * \brief Indicates whether this device has a Unified Memory Architecture
         *
         * UMA devices don't need to use a transfer queue to upload data, they can map a pointer directly to all resources
         */
        bool is_uma = false;

        /*!
         * \brief Indicates the level of hardware and driver support for render passes
         *
         * Tier 0 - No support, don't use renderpasses
         * Tier 1 - render targets and depth/stencil writes should use renderpasses, but UAV writes are not supported
         * Tire 2 - render targets, depth/stencil, and UAV writes should use renderpasses
         */
        D3D12_RENDER_PASS_TIER render_pass_tier = D3D12_RENDER_PASS_TIER_0;

        /*!
         * \brief Indicates support the the DXR API
         *
         * If this is `false`, the user will be unable to use any DXR shaderpacks
         */
        bool has_raytracing = false;

        DXGI_FORMAT swapchain_format{DXGI_FORMAT_R8G8B8A8_UNORM};

        std::vector<ComPtr<ID3D12Fence>> command_list_done_fences;

        std::vector<D3D12BindGroupBuilder> material_bind_group_builder;

        /*!
         * \brief Index of the swapchain image we're currently rendering to
         */
        UINT cur_swapchain_idx{0};

        /*!
         * \brief Index of the GPU frame we're currently recording
         */
        uint32_t cur_gpu_frame_idx{0};

        /*!
         * \brief Description for a point sampler
         */
        D3D12_STATIC_SAMPLER_DESC point_sampler_desc;

        /*!
         * \brief Description for a linear sampler
         */
        D3D12_STATIC_SAMPLER_DESC linear_sampler_desc;

#pragma region initialization
        void enable_debugging();

        void initialize_dxgi();

        void select_adapter();

        void create_queues();

        void create_swapchain(HWND window_handle, const XMINT2& window_size, UINT num_images);

        void create_gpu_frame_synchronization_objects();

        void create_command_allocators();

        void create_descriptor_heaps();

        void initialize_swapchain_descriptors();

        [[nodiscard]] std::pair<ID3D12DescriptorHeap*, UINT> create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
                                                                                    uint32_t num_descriptors) const;

        void initialize_dma();

        void create_static_sampler_descriptions();

        void create_standard_root_signature();

        [[nodiscard]] ComPtr<ID3D12RootSignature> compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const;

        void create_material_resource_binders();

        void create_standard_graphics_pipeline_input_layout();
#pragma endregion

        [[nodiscard]] std::vector<D3D12_SHADER_INPUT_BIND_DESC> get_bindings_from_shader(const std::vector<uint8_t>& shader) const;

        [[nodiscard]] std::unique_ptr<RenderPipelineState> create_pipeline_state(const RenderPipelineStateCreateInfo& create_info,
                                                                                 ID3D12RootSignature& root_signature);

        void return_staging_buffers_for_frame(uint32_t frame_idx);

        void reset_command_allocators_for_frame(uint32_t frame_idx);

        void wait_for_frame(uint64_t frame_index);

        void wait_gpu_idle(uint64_t frame_index);

        [[nodiscard]] D3D12StagingBuffer create_staging_buffer(uint32_t num_bytes);

        [[nodiscard]] ComPtr<ID3D12Fence> get_next_command_list_done_fence();

        void retrieve_dred_report();
    };
} // namespace rhi

#pragma once

#include <memory>

#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#include <glm/glm.hpp>
#include <wrl/client.h>

// Must be this low in the include order, because this header doesn't include all the things it needs
#include <DXProgrammableCapture.h>
#include <TracyD3D12.hpp>
#include <ftl/fibtex.h>

#include "rhi/bind_group.hpp"
#include "rhi/compute_pipeline_state.hpp"
#include "rhi/d3dx12.hpp"
#include "rhi/descriptor_allocator.hpp"
#include "rhi/framebuffer.hpp"
#include "rhi/raytracing_structs.hpp"
#include "rhi/render_pipeline_state.hpp"
#include "settings.hpp"

struct GLFWwindow;

using Microsoft::WRL::ComPtr;

namespace D3D12MA {
    class Allocator;
}

namespace renderer {
    enum class BespokePipelineType;
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
        inline static Uint32 material_buffer_root_parameter_index = 2;
#ifdef TRACY_ENABLE
        inline static tracy::D3D12QueueCtx* tracy_context;
#endif

        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12Device1> device1;
        ComPtr<ID3D12Device5> device5;

        RenderDevice(HWND window_handle, const glm::uvec2& window_size, const Settings& settings_in, ftl::TaskScheduler& task_scheduler);

        RenderDevice(const RenderDevice& other) = delete;
        RenderDevice& operator=(const RenderDevice& other) = delete;

        RenderDevice(RenderDevice&& old) noexcept = delete;
        RenderDevice& operator=(RenderDevice&& old) noexcept = delete;

        ~RenderDevice();

#pragma region RenderDevice
        [[nodiscard]] Rx::Ptr<Buffer> create_buffer(const BufferCreateInfo& create_info) const;

        [[nodiscard]] Rx::Ptr<Image> create_image(const ImageCreateInfo& create_info) const;

        [[nodiscard]] Rx::Ptr<Framebuffer> create_framebuffer(const Rx::Vector<const Image*>& render_targets,
                                                              const Image* depth_target = nullptr) const;

        Framebuffer* get_backbuffer_framebuffer();

        [[nodiscard]] void* map_buffer(const Buffer& buffer) const;

        void schedule_buffer_destruction(Rx::Ptr<Buffer> buffer);

        void schedule_image_destruction(Rx::Ptr<Image> image);

        void destroy_framebuffer(Rx::Ptr<Framebuffer> framebuffer) const;

        /*!
         * \brief Creates a bind group builder with the provided descriptors
         *
         * \param root_descriptors Mapping from root descriptor name to information about how to bind to that root descriptor
         * \param descriptor_table_descriptors Mapping from the name of a descriptors in a descriptor table to information about how to bind
         * to that descriptors
         * \param descriptor_table_handles Mapping from root parameters index to GPU handle to the descriptor table to bind to that index
         */
        [[nodiscard]] Rx::Ptr<BindGroupBuilder> create_bind_group_builder(
            const Rx::Map<Rx::String, RootDescriptorDescription>& root_descriptors = {},
            const Rx::Map<Rx::String, DescriptorTableDescriptorDescription>& descriptor_table_descriptors = {},
            const Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE>& descriptor_table_handles = {});

        [[nodiscard]] Rx::Ptr<ComputePipelineState> create_compute_pipeline_state(const Rx::Vector<uint8_t>& compute_shader,
                                                                                  const ComPtr<ID3D12RootSignature>& root_signature) const;

        [[nodiscard]] Rx::Ptr<RenderPipelineState> create_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info);

        void destroy_compute_pipeline_state(Rx::Ptr<ComputePipelineState> pipeline_state);

        void destroy_render_pipeline_state(Rx::Ptr<RenderPipelineState> pipeline_state);

        [[nodiscard]] ComPtr<ID3D12GraphicsCommandList4> create_command_list(Size thread_idx);

        void submit_command_list(const ComPtr<ID3D12GraphicsCommandList4>& commands);

        BindGroupBuilder& get_material_bind_group_builder_for_frame(Uint32 frame_idx);

        void begin_frame(uint64_t frame_count, Size thread_idx);

        void end_frame(Size thread_idx);

        [[nodiscard]] Uint32 get_cur_gpu_frame_idx() const;

        void begin_capture() const;

        void end_capture() const;
#pragma endregion

        [[nodiscard]] bool has_separate_device_memory() const;

        [[nodiscard]] StagingBuffer get_staging_buffer(Uint32 num_bytes);

        void return_staging_buffer(StagingBuffer&& buffer);

        [[nodiscard]] Buffer get_scratch_buffer(Uint32 num_bytes);

        void return_scratch_buffer(Buffer&& buffer);

        [[nodiscard]] ID3D12Device* get_d3d12_device() const;

        [[nodiscard]] UINT get_shader_resource_descriptor_size() const;

        [[nodiscard]] ComPtr<ID3D12RootSignature> compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const;

        /*!
         * \brief Allocated a descriptor table with the specified number of descriptors, returning both a CPU and GPU handle to the start of
         * the table. All descriptors in the table are tightly packed
         */
        [[nodiscard]] std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> allocate_descriptor_table(
            Uint32 num_descriptors);

    private:
        Settings settings;

        /*!
         * \brief Marker for if the engine is still being initialized and hasn't yet rendered any frame
         *
         * This allows the renderer to queue up work to be executed on the first frame
         */
        bool in_init_phase{true};

        ComPtr<ID3D12Debug> debug_controller;
        ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dred_settings;
        ComPtr<IDXGraphicsAnalysis> graphics_analysis;

        ComPtr<IDXGIFactory4> factory;

        ComPtr<IDXGIAdapter> adapter;

        ComPtr<ID3D12InfoQueue> info_queue;

        ComPtr<ID3D12CommandQueue> direct_command_queue;

        ComPtr<ID3D12CommandQueue> async_copy_queue;

        ftl::Fibtex direct_command_allocators_fibtex;
        Rx::Vector<Rx::Map<Size, ComPtr<ID3D12CommandAllocator>>> direct_command_allocators;

        Rx::Vector<ComPtr<ID3D12CommandAllocator>> compute_command_allocators;

        Rx::Vector<ComPtr<ID3D12CommandAllocator>> copy_command_allocators;

        ftl::Fibtex command_lists_by_frame_fibtex;
        Rx::Vector<Rx::Vector<ComPtr<ID3D12GraphicsCommandList4>>> command_lists_by_frame;

        ComPtr<IDXGISwapChain3> swapchain;
        Rx::Vector<ComPtr<ID3D12Resource>> swapchain_images;
        Rx::Vector<Framebuffer> swapchain_framebuffers;

        HANDLE frame_event;
        ComPtr<ID3D12Fence> frame_fences;
        Rx::Vector<uint64_t> frame_fence_values;

        Rx::Vector<Rx::Vector<Rx::Ptr<Buffer>>> buffer_deletion_list;
        Rx::Vector<Rx::Vector<Rx::Ptr<Image>>> image_deletion_list;

        ComPtr<ID3D12DescriptorHeap> cbv_srv_uav_heap;
        UINT cbv_srv_uav_size{};
        UINT next_free_cbv_srv_uav_descriptor{0};

        Rx::Ptr<DescriptorAllocator> rtv_allocator;

        Rx::Ptr<DescriptorAllocator> dsv_allocator;

        D3D12MA::Allocator* device_allocator;

        ComPtr<ID3D12RootSignature> standard_root_signature;

        Rx::Vector<D3D12_INPUT_ELEMENT_DESC> standard_graphics_pipeline_input_layout;
        Rx::Vector<D3D12_INPUT_ELEMENT_DESC> dear_imgui_graphics_pipeline_input_layout;

        uint64_t staging_buffer_idx{0};
        Rx::Vector<StagingBuffer> staging_buffers;

        /*!
         * \brief Array of array of staging buffers to free on a frame. index 0 gets freed on the next frame 0, index 1 gets freed on the
         * next frame 1, etc
         */
        Rx::Vector<Rx::Vector<StagingBuffer>> staging_buffers_to_free;

        Uint32 scratch_buffer_counter{0};
        Rx::Vector<Buffer> scratch_buffers;
        Rx::Vector<Rx::Vector<Buffer>> scratch_buffers_to_free;

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

        Rx::Vector<ComPtr<ID3D12Fence>> command_list_done_fences;

        Rx::Vector<Rx::Ptr<BindGroupBuilder>> material_bind_group_builder;

        /*!
         * \brief Index of the swapchain image we're currently rendering to
         */
        UINT cur_swapchain_idx{0};

        /*!
         * \brief Index of the GPU frame we're currently recording
         */
        Uint32 cur_gpu_frame_idx{0};

        /*!
         * \brief Description for a point sampler
         */
        D3D12_STATIC_SAMPLER_DESC point_sampler_desc{.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
                                                     .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                     .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                     .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                     .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
                                                     .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL};

        /*!
         * \brief Description for a linear sampler
         */
        D3D12_STATIC_SAMPLER_DESC linear_sampler_desc{.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                                                      .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                      .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                      .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                      .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
                                                      .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL};

#pragma region initialization
        void enable_debugging();

        void initialize_dxgi();

        void select_adapter();

        void create_queues();

        void create_swapchain(HWND window_handle, const glm::uvec2& window_size, UINT num_images);

        void create_gpu_frame_synchronization_objects();

        void create_command_allocators();

        void create_descriptor_heaps();

        void initialize_swapchain_descriptors();

        [[nodiscard]] std::pair<ID3D12DescriptorHeap*, UINT> create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
                                                                                    Uint32 num_descriptors) const;

        void initialize_dma();

        void create_standard_root_signature();

        void create_material_resource_binders();

        void create_pipeline_input_layouts();
#pragma endregion

        [[nodiscard]] Rx::Vector<D3D12_SHADER_INPUT_BIND_DESC> get_bindings_from_shader(const Rx::Vector<uint8_t>& shader) const;

        [[nodiscard]] Rx::Ptr<RenderPipelineState> create_pipeline_state(const RenderPipelineStateCreateInfo& create_info,
                                                                         ID3D12RootSignature& root_signature);

        [[nodiscard]] ID3D12CommandAllocator* get_direct_command_allocator_for_thread(Size id);

        void flush_batched_command_lists();

        void return_staging_buffers_for_frame(Uint32 frame_idx);

        void reset_command_allocators_for_frame(Uint32 frame_idx);

        template <GpuResource ResourceType>
        void destroy_resource_immediate(const ResourceType& resource);

        void destroy_resources_for_frame(Uint32 frame_idx);

        void transition_swapchain_image_to_render_target(Size thread_idx);

        void transition_swapchain_image_to_presentable(Size thread_idx);

        void wait_for_frame(uint64_t frame_index);

        void wait_gpu_idle(uint64_t frame_index);

        [[nodiscard]] StagingBuffer create_staging_buffer(Uint32 num_bytes);

        [[nodiscard]] Buffer create_scratch_buffer(Uint32 num_bytes);

        [[nodiscard]] ComPtr<ID3D12Fence> get_next_command_list_done_fence();

        void retrieve_dred_report() const;
    };

    [[nodiscard]] Rx::Ptr<RenderDevice> make_render_device(RenderBackend backend, GLFWwindow* window, const Settings& settings);

    template <GpuResource ResourceType>
    void RenderDevice::destroy_resource_immediate(const ResourceType& resource) {
        resource.resource->Release();
    }
} // namespace renderer

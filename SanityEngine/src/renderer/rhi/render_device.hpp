#pragma once

#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>

// Must be this low in the include order, because this header doesn't include all the things it needs
#include <DXProgrammableCapture.h>

#include "core/Prelude.hpp"
#include "core/types.hpp"
#include "glm/glm.hpp"
#include "renderer/rhi/bind_group.hpp"
#include "renderer/rhi/d3dx12.hpp"
#include "renderer/rhi/descriptor_allocator.hpp"
#include "renderer/rhi/framebuffer.hpp"
#include "renderer/rhi/raytracing_structs.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/console/variable.h"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/ptr.h"
#include "settings.hpp"

struct GLFWwindow;

namespace D3D12MA {
    class Allocator;
}

#ifdef TRACY_ENABLE
namespace tracy {
    class D3D12QueueCtx;
}
#endif

namespace sanity::engine::renderer {
    struct CommandList;
    struct RenderPipelineStateCreateInfo;

    struct __declspec(uuid("5A6A1D35-71A1-4DF5-81AA-EF05E0492280")) GpuFrameIdx {
        Uint32 idx;
    };

    /*
     * \brief A device which can be used to render
     *
     * A render device - and by extension the CPU - may only record commands for a single frame at a time. However, the GPU may be executing
     * one frame for each image in the swapchain. Most of the synchronization concerns should be hidden behind this interface, but be aware
     * that the frame that the GPU may be several frames behind the CPU
     */
    class RenderBackend {
    public:
        static constexpr Uint32 CAMERA_INDEX_ROOT_CONSTANT_OFFSET = 0;
        static constexpr Uint32 MATERIAL_INDEX_ROOT_CONSTANT_OFFSET = 1;
        static constexpr Uint32 MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET = 2;
        static constexpr Uint32 OBJECT_ID_ROOT_CONSTANT_OFFSET = 3;

        static constexpr Uint32 ROOT_CONSTANTS_ROOT_PARAMETER_INDEX = 0;
        static constexpr Uint32 MATERIAL_BUFFER_ROOT_PARAMETER_INDEX = 2;
        static constexpr Uint32 MODEL_MATRIX_BUFFER_ROOT_PARAMETER_INDEX = 8;

#ifdef TRACY_ENABLE
        inline static tracy::D3D12QueueCtx* tracy_context{nullptr};
#endif

        // TODO: Express this struct in compute land
        struct IndirectDrawCommandWithRootConstant {
            Uint32 constant{};
            Uint32 vertex_count{};
            Uint32 instance_count{1};
            Uint32 start_index_location{};
            Int32 base_vertex_location{};
            Uint32 start_instance_location{};
        };

        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12Device1> device1;
        ComPtr<ID3D12Device5> device5;

        RenderBackend(HWND window_handle, const glm::uvec2& window_size);

        RenderBackend(const RenderBackend& other) = delete;
        RenderBackend& operator=(const RenderBackend& other) = delete;

        RenderBackend(RenderBackend&& old) noexcept = delete;
        RenderBackend& operator=(RenderBackend&& old) noexcept = delete;

        ~RenderBackend();

        [[nodiscard]] Rx::Ptr<Buffer> create_buffer(const BufferCreateInfo& create_info,
                                                    D3D12_RESOURCE_FLAGS additional_flags = D3D12_RESOURCE_FLAG_NONE) const;

        [[nodiscard]] Rx::Ptr<Image> create_image(const ImageCreateInfo& create_info) const;

        [[nodiscard]] DescriptorRange create_rtv_handle(const Image& image) const;

        [[nodiscard]] DescriptorRange create_dsv_handle(const Image& image) const;

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_backbuffer_rtv_handle();

        [[nodiscard]] Uint2 get_backbuffer_size() const;

        [[nodiscard]] void* map_buffer(const Buffer& buffer) const;

        void schedule_buffer_destruction(Rx::Ptr<Buffer> buffer);

        void schedule_image_destruction(Rx::Ptr<Image> image);

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
            const Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE>& descriptor_table_handles = {}) const;

        [[nodiscard]] ComPtr<ID3D12PipelineState> create_compute_pipeline_state(const Rx::Vector<Uint8>& compute_shader,
                                                                                const ComPtr<ID3D12RootSignature>& root_signature) const;

        [[nodiscard]] Rx::Ptr<RenderPipelineState> create_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info);

        /*!
         * \brief Creates a new command list
         *
         * You may pass in the index of the GPU frame to submit this command list to. If you do not, the index of the GPU frame currently
         * being recorded is used
         *
         * This method is internally synchronized. You can (in theory) call it safely from a multithreaded environment
         */
        [[nodiscard]] ComPtr<ID3D12GraphicsCommandList4> create_command_list(Rx::Optional<Uint32> frame_idx = Rx::nullopt);

        void submit_command_list(ComPtr<ID3D12GraphicsCommandList4>&& commands);

        BindGroupBuilder& get_material_bind_group_builder_for_frame(Uint32 frame_idx);

        void begin_frame(uint64_t frame_count);

        void end_frame();

        [[nodiscard]] Uint32 get_cur_gpu_frame_idx() const;

        /*!
         * @brief Begins a programmatic capture which must be manually ended
         */
        void begin_capture() const;

        /*!
         * @brief Begins a programmatic capture that will end after the frame has been presented
         */
        void begin_frame_capture();

        void end_capture() const;

        void wait_idle();

        [[nodiscard]] Uint32 get_max_num_gpu_frames() const;

        [[nodiscard]] bool has_separate_device_memory() const;

        [[nodiscard]] Buffer get_staging_buffer(Uint64 num_bytes);

        [[nodiscard]] Buffer get_staging_buffer_for_texture(ID3D12Resource* texture);

        void return_staging_buffer(const Buffer& buffer);

        [[nodiscard]] Buffer get_scratch_buffer(Uint32 num_bytes);

        void return_scratch_buffer(const Buffer& buffer);

        [[nodiscard]] ID3D12Device* get_d3d12_device() const;

        [[nodiscard]] ComPtr<ID3D12RootSignature> compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const;

        [[nodiscard]] DescriptorAllocator& get_cbv_srv_uav_allocator() const;

        [[nodiscard]] ID3D12DescriptorHeap* get_cbv_srv_uav_heap() const;

    private:
        Settings settings;

        /*!
         * \brief Marker for if the engine is still being initialized and hasn't yet rendered any frame
         *
         * This allows the renderer to queue up work to be executed on the first frame
         */
        bool in_init_phase{true};

        ComPtr<ID3D12Debug1> debug_controller;
        ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred_settings;
        ComPtr<IDXGraphicsAnalysis> graphics_analysis;

        bool is_frame_capture_active{false};

        ComPtr<IDXGIFactory4> factory;

        ComPtr<IDXGIAdapter> adapter;

        ComPtr<ID3D12InfoQueue> info_queue;
        DWORD debug_message_callback_cookie;

        ComPtr<ID3D12CommandQueue> direct_command_queue;

        ComPtr<ID3D12CommandQueue> async_copy_queue;

        Rx::Concurrency::Mutex create_command_list_mutex;
        Rx::Concurrency::Atomic<Size> command_lists_outside_render_device{0};

        Rx::Concurrency::Mutex direct_command_allocators_mutex;
        Rx::Vector<ComPtr<ID3D12CommandAllocator>> direct_command_allocators;

        Rx::Concurrency::Mutex command_lists_by_frame_mutex;
        Rx::Vector<Rx::Vector<ComPtr<ID3D12GraphicsCommandList4>>> command_lists_to_submit_on_end_frame;
        Rx::Vector<Rx::Vector<ComPtr<ID3D12CommandAllocator>>> command_allocators_to_reset_on_begin_frame;

        ComPtr<IDXGISwapChain3> swapchain;
        Rx::Vector<ComPtr<ID3D12Resource>> swapchain_images;
        Rx::Vector<DescriptorRange> swapchain_rtv_handles;

        HANDLE frame_event;
        ComPtr<ID3D12Fence> frame_fences;
        Rx::Vector<uint64_t> frame_fence_values;

        Rx::Vector<Rx::Vector<Rx::Ptr<Buffer>>> buffer_deletion_list;
        Rx::Vector<Rx::Vector<Rx::Ptr<Image>>> image_deletion_list;

        Rx::Ptr<DescriptorAllocator> cbv_srv_uav_allocator;
        Rx::Ptr<DescriptorAllocator> rtv_allocator;
        Rx::Ptr<DescriptorAllocator> dsv_allocator;

        D3D12MA::Allocator* device_allocator;

        ComPtr<ID3D12RootSignature> standard_root_signature;

        Rx::Vector<D3D12_INPUT_ELEMENT_DESC> standard_graphics_pipeline_input_layout;
        Rx::Vector<D3D12_INPUT_ELEMENT_DESC> dear_imgui_graphics_pipeline_input_layout;

        uint64_t staging_buffer_idx{0};
        Rx::Vector<Buffer> staging_buffers;

        /*!
         * \brief Array of array of staging buffers to free on a frame. index 0 gets freed on the next frame 0, index 1 gets freed on the
         * next frame 1, etc
         */
        Rx::Vector<Rx::Vector<Buffer>> staging_buffers_to_free;

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

        ComPtr<ID3D12CommandSignature> standard_drawcall_command_signature;

#pragma region initialization
        void enable_debugging();

        void initialize_dxgi();

        void select_adapter();

        void create_queues();

        void create_swapchain(HWND window_handle, const glm::uvec2& window_size);

        void create_gpu_frame_synchronization_objects();

        void create_command_allocators();

        void create_descriptor_heaps();

        void initialize_swapchain_descriptors();

        [[nodiscard]] std::pair<ComPtr<ID3D12DescriptorHeap>, UINT> create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
                                                                                           Uint32 num_descriptors) const;

        void initialize_dma();

        void create_standard_root_signature();

        void create_material_resource_binders();

        void create_pipeline_input_layouts();

        void create_command_signatures();
#pragma endregion

        [[nodiscard]] Rx::Vector<D3D12_SHADER_INPUT_BIND_DESC> get_bindings_from_shader(const Rx::Vector<Uint8>& shader) const;

        [[nodiscard]] Rx::Ptr<RenderPipelineState> create_pipeline_state(const RenderPipelineStateCreateInfo& create_info,
                                                                         ID3D12RootSignature& root_signature);

        void flush_batched_command_lists();

        void return_staging_buffers_for_frame(Uint32 frame_idx);

        void reset_command_allocators_for_frame(Uint32 frame_idx);

        template <GpuResource ResourceType>
        void destroy_resource_immediate(const ResourceType& resource);

        void destroy_resources_for_frame(Uint32 frame_idx);

        void transition_swapchain_image_to_render_target();

        void transition_swapchain_image_to_presentable();

        void wait_for_frame(uint64_t frame_index);

        void wait_gpu_idle(uint64_t frame_index);

        [[nodiscard]] Buffer create_staging_buffer(Uint64 num_bytes);

        [[nodiscard]] Buffer create_scratch_buffer(Uint32 num_bytes);

        [[nodiscard]] ComPtr<ID3D12Fence> get_next_command_list_done_fence();

        void log_dred_report() const;
    };

    [[nodiscard]] Rx::Ptr<RenderBackend> make_render_device(GLFWwindow* window);
} // namespace sanity::engine::renderer

#pragma once

#include <rx/core/assert.h>
#define D3D12MA_ASSERT(cond) RX_ASSERT(cond)

#define interface struct

#include <D3D12MemAlloc.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <rx/core/map.h>
#include <rx/core/utility/pair.h>
#include <rx/math/vec2.h>

#define interface struct
#include <wrl/client.h>

#include "../renderer.hpp"
#include "d3d12_descriptor_allocator.hpp"
#include "resources.hpp"

namespace render {
    using Microsoft::WRL::ComPtr;

    class D3D12RenderDevice : public virtual RenderDevice {
    public:
        D3D12RenderDevice(rx::memory::allocator& allocator, HWND window_handle, const rx::math::vec2i& window_size);

        D3D12RenderDevice(const D3D12RenderDevice& other) = delete;
        D3D12RenderDevice& operator=(const D3D12RenderDevice& other) = delete;

        D3D12RenderDevice(D3D12RenderDevice&& old) noexcept = delete;
        D3D12RenderDevice& operator=(D3D12RenderDevice&& old) noexcept = delete;

        ~D3D12RenderDevice() override;

#pragma region RenderDevice
        [[nodiscard]] rx::ptr<Buffer> create_buffer(rx::memory::allocator& allocator, const BufferCreateInfo& create_info) override;

        [[nodiscard]] rx::ptr<Image> create_image(rx::memory::allocator& allocator, const ImageCreateInfo& create_info) override;

        [[nodiscard]] rx::ptr<Framebuffer> create_framebuffer(const rx::vector<const Image*>& render_targets,
                                                              const Image* depth_target) override;

        void* map_buffer(const Buffer& buffer) override;

        void destroy_buffer(rx::ptr<Buffer> buffer) override;

        void destroy_image(rx::ptr<Image> image) override;

        void destroy_framebuffer(rx::ptr<Framebuffer> framebuffer) override;

        [[nodiscard]] rx::ptr<ComputePipelineState> create_compute_pipeline_state() override;

        [[nodiscard]] rx::ptr<RenderPipelineState> create_render_pipeline_state() override;

        void destroy_compute_pipeline_state(rx::ptr<ComputePipelineState> pipeline_state) override;

        void destroy_render_pipeline_state(rx::ptr<RenderPipelineState> pipeline_state) override;

        [[nodiscard]] rx::ptr<ResourceCommandList> create_resource_command_list() override;

        [[nodiscard]] rx::ptr<ComputeCommandList> create_compute_command_list() override;

        [[nodiscard]] rx::ptr<RenderCommandList> create_render_command_list() override;

        void submit_command_list(rx::ptr<CommandList> commands) override;

        void begin_frame() override;
#pragma endregion

        [[nodiscard]] bool has_separate_device_memory() const;

        [[nodiscard]] rx::ptr<D3D12StagingBuffer> get_staging_buffer(size_t num_bytes);

        void return_staging_buffer(rx::ptr<D3D12StagingBuffer> buffer);

        [[nodiscard]] auto* get_d3d12_device() const;

        [[nodiscard]] auto get_shader_resource_descriptor_size() const;

        [[nodiscard]] ComPtr<ID3D12Fence> get_next_command_list_done_fence();

    private:
        rx::memory::allocator* internal_allocator;

        ComPtr<ID3D12Debug> debug_controller;

        ComPtr<IDXGIFactory4> factory;

        ComPtr<IDXGIAdapter> adapter;

        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12Device1> device1;

        ComPtr<ID3D12InfoQueue> info_queue;

        ComPtr<ID3D12CommandQueue> direct_command_queue;

        ComPtr<ID3D12CommandQueue> async_copy_queue;

        ComPtr<ID3D12CommandAllocator> direct_command_allocator;

        ComPtr<ID3D12CommandAllocator> compute_command_allocator;

        ComPtr<ID3D12CommandAllocator> copy_command_allocator;

        ComPtr<IDXGISwapChain3> swapchain;

        ComPtr<ID3D12DescriptorHeap> cbv_srv_uav_heap;
        UINT cbv_srv_uav_size{};

        rx::ptr<D3D12DescriptorAllocator> rtv_allocator;

        rx::ptr<D3D12DescriptorAllocator> dsv_allocator;

        D3D12MA::Allocator* device_allocator;

        ComPtr<IDxcLibrary> dxc_library;

        ComPtr<IDxcCompiler> dxc_compiler;

        ComPtr<ID3D12RootSignature> standard_root_signature;

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

        rx::vector<ComPtr<ID3D12Fence>> command_list_done_fences;

#pragma region initialization
        void enable_validation_layer();

        void initialize_dxgi();

        void select_adapter();

        void create_queues();

        void create_swapchain(HWND window_handle, const rx::math::vec2i& window_size, UINT num_images);

        void create_command_allocators();

        void create_descriptor_heaps();

        [[nodiscard]] rx::pair<ComPtr<ID3D12DescriptorHeap>, UINT> create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
                                                                                               uint32_t num_descriptors) const;

        void initialize_dma();

        void create_shader_compiler();

        void create_standard_root_signature();

        [[nodiscard]] ComPtr<ID3D12RootSignature> compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const;

        void create_material_resource_binder();
#pragma endregion
    };
} // namespace render

#pragma once

#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <rx/math/vec2.h>
#include <wrl/client.h>

#include "../renderer.hpp"
#include "rx/core/utility/pair.h"

namespace render {
    using Microsoft::WRL::ComPtr;

    class D3D12RenderDevice : public virtual RenderDevice {
    public:
        D3D12RenderDevice(rx::memory::allocator& allocator, HWND window_handle, const rx::math::vec2i& window_size);

        ~D3D12RenderDevice() override;

#pragma region RenderDevice
        [[nodiscard]] rx::ptr<ResourceCommandList> get_resource_command_list() override;

        [[nodiscard]] rx::ptr<ComputeCommandList> get_compute_command_list() override;

        [[nodiscard]] rx::ptr<GraphicsCommandList> get_graphics_command_list() override;

        void submit_command_list(rx::ptr<CommandList> commands) override;
#pragma endregion

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

        ComPtr<IDXGISwapChain3> swapchain;

        ComPtr<ID3D12DescriptorHeap> cbv_srv_uav_heap;
        UINT cbv_srv_uav_size;

        ComPtr<ID3D12DescriptorHeap> rtv_heap;
        UINT rtv_size;

        ComPtr<ID3D12DescriptorHeap> dsv_heap;
        UINT dsv_size;

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

#pragma region initialization
        void enable_validation_layer();

        void initialize_dxgi();

        void select_adapter();

        void create_queues();

        void create_swapchain(HWND window_handle, const rx::math::vec2i& window_size, UINT num_images);

        void create_command_allocators();

        void create_descriptor_heaps();

        rx::pair<ComPtr<ID3D12DescriptorHeap>, UINT> create_descriptor_allocator(
            D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, uint32_t num_descriptors) const;
#pragma endregion
    };
} // namespace render

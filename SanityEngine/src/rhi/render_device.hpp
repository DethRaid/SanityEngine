#pragma once

#include <d3d11.h>
#include <dxgi1_4.h>
#include <glm/glm.hpp>
#include <rx/console/variable.h>
#include <rx/core/concurrency/mutex.h>
#include <wrl/client.h>

// Must be this low in the include order, because this header doesn't include all the things it needs
#include <DXProgrammableCapture.h>

#include "rhi/bind_group.hpp"
#include "rhi/framebuffer.hpp"
#include "rhi/raytracing_structs.hpp"
#include "settings.hpp"
#include "core/types.hpp"
#include "core/async/synchronized_resource.hpp"

struct GLFWwindow;

using Microsoft::WRL::ComPtr;

namespace renderer {
    struct RenderPipelineStateCreateInfo;

    /*
     * \brief A device which can be used to render
     *
     * A render device - and by extension the CPU - may only record commands for a single frame at a time. However, the GPU may be executing
     * one frame for each image in the swapchain. Most of the synchronization concerns should be hidden behind this interface, but be aware
     * that the frame that the GPU may be several frames behind the CPU
     */
    class RenderDevice {
    public:
        static constexpr Uint32 CAMERA_INDEX_ROOT_CONSTANT_OFFSET = 0;
        static constexpr Uint32 MATERIAL_INDEX_ROOT_CONSTANT_OFFSET = 1;
        static constexpr Uint32 MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET = 2;

        static constexpr Uint32 MATERIAL_BUFFER_ROOT_PARAMETER_INDEX = 1;
        static constexpr Uint32 MODEL_MATRIX_BUFFER_ROOT_PARAMETER_INDEX = 7;

        ComPtr<ID3D11Device> device;
        SynchronizedResource<ComPtr<ID3D11DeviceContext>> device_context;

        RenderDevice(HWND window_handle, const glm::uvec2& window_size, const Settings& settings_in);

        RenderDevice(const RenderDevice& other) = delete;
        RenderDevice& operator=(const RenderDevice& other) = delete;

        RenderDevice(RenderDevice&& old) noexcept = delete;
        RenderDevice& operator=(RenderDevice&& old) noexcept = delete;

        ~RenderDevice();

        [[nodiscard]] Rx::Ptr<Buffer> create_buffer(const BufferCreateInfo& create_info);

        [[nodiscard]] Rx::Ptr<Image> create_image(const ImageCreateInfo& create_info) const;

        [[nodiscard]] ComPtr<ID3D11RenderTargetView> create_rtv(const Image& image) const;

        [[nodiscard]] ComPtr<ID3D11DepthStencilView> create_dsv(const Image& image) const;

        [[nodiscard]] ComPtr<ID3D11RenderTargetView> get_backbuffer_rtv() const;

        [[nodiscard]] Vec2u get_backbuffer_size() const;

        [[nodiscard]] bool map_buffer(Buffer& buffer);

        [[nodiscard]] SynchronizedResourceAccessor<ComPtr<ID3D11DeviceContext>> get_device_context();

        void begin_frame();

        void end_frame() const;

        void begin_capture() const;

        void end_capture() const;

        [[nodiscard]] Uint32 get_max_num_gpu_frames() const;

        [[nodiscard]] bool has_separate_device_memory() const;

        [[nodiscard]] Buffer get_staging_buffer(Uint32 num_bytes);

        void return_staging_buffer(Buffer&& buffer);

        [[nodiscard]] ID3D11Device* get_d3d11_device() const;

    private:
        Settings settings;

        /*!
         * \brief Marker for if the engine is still being initialized and hasn't yet rendered any frame
         *
         * This allows the renderer to queue up work to be executed on the first frame
         */
        bool in_init_phase{true};

        ComPtr<IDXGraphicsAnalysis> graphics_analysis;

        ComPtr<IDXGIFactory4> factory;

        ComPtr<IDXGIAdapter> adapter;

        ComPtr<ID3D11InfoQueue> info_queue;

        ComPtr<IDXGISwapChain3> swapchain;
        Rx::Vector<ComPtr<ID3D11Texture2D>> swapchain_images;
        Rx::Vector<ComPtr<ID3D11RenderTargetView>> swapchain_rtvs;

        uint64_t staging_buffer_idx{0};
        Rx::Vector<Buffer> staging_buffers;

        /*!
         * \brief Indicates whether this device has a Unified Memory Architecture
         *
         * UMA devices don't need to use a transfer queue to upload data, they can map a pointer directly to all resources
         */
        bool is_uma = false;

        DXGI_FORMAT swapchain_format{DXGI_FORMAT_R8G8B8A8_UNORM};

        /*!
         * \brief Index of the swapchain image we're currently rendering to
         */
        UINT cur_swapchain_idx{0};

#pragma region initialization
        void enable_debugging();

        void initialize_dxgi();

        void create_device_and_swapchain(const glm::uvec2& window_size);
#pragma endregion

        [[nodiscard]] Buffer create_staging_buffer(Uint32 num_bytes);
    };

    [[nodiscard]] Rx::Ptr<RenderDevice> make_render_device(GLFWwindow* window, const Settings& settings);
} // namespace renderer

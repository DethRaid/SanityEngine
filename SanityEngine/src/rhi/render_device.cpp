#include "render_device.hpp"

#include <rx/core/string.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <algorithm>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>

#include "adapters/tracy.hpp"
#include "core/errors.hpp"
#include "rhi/helpers.hpp"
#include "settings.hpp"

namespace renderer {
    RX_LOG("RenderDevice", logger);

    RX_CONSOLE_IVAR(
        cvar_max_in_flight_gpu_frames, "r.MaxInFlightGpuFrames", "Maximum number of frames that the GPU may work on concurrently", 0, 8, 3);

    RX_CONSOLE_BVAR(cvar_break_on_validation_error,
                    "r.BreakOnValidationError",
                    "Whether to issue a breakpoint when the validation layer encounters an error",
                    false);

    RenderDevice::RenderDevice(HWND window_handle, // NOLINT(cppcoreguidelines-pro-type-member-init)
                               const glm::uvec2& window_size,
                               const Settings& settings_in)
        : settings{settings_in} {
#ifndef NDEBUG
        // Only enable the debug layer if we're not running in PIX
        const auto result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&graphics_analysis));
        if(FAILED(result)) {
            enable_debugging();
        }
#endif

        initialize_dxgi();

        create_device_and_swapchain(window_size);

        logger->info("Initialized D3D12 render device");
    }

    RenderDevice::~RenderDevice() {
        auto locked_context = device_context.lock();
        (*locked_context)->Flush();
    }

    Rx::Ptr<Buffer> RenderDevice::create_buffer(const BufferCreateInfo& create_info) {
        ZoneScoped;
        auto desc = D3D11_BUFFER_DESC{
            .ByteWidth = create_info.size,
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = 0,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if(create_info.usage == BufferUsage::StagingBuffer) {
            // Try to get a staging buffer from the pool
        }

        bool should_map = false;

        switch(create_info.usage) {
            case BufferUsage::StagingBuffer:
                desc.BindFlags = 0;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                should_map = true;
                break;

            case BufferUsage::IndexBuffer:
                desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                break;

            case BufferUsage::VertexBuffer:
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                break;

            case BufferUsage::ConstantBuffer:
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                should_map = true;
                break;

            case BufferUsage::IndirectCommands:
                [[fallthrough]];
            case BufferUsage::UnorderedAccess:
                desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
                break;

            case BufferUsage::UiVertices:
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
                break;
        }

        auto buffer = Rx::make_ptr<Buffer>(RX_SYSTEM_ALLOCATOR);
        const auto result = device->CreateBuffer(&desc, nullptr, &buffer->resource);
        if(FAILED(result)) {
            logger->error("Could not create buffer %s: %s", create_info.name, to_string(result));
            return {};
        }

        set_object_name(buffer->resource.Get(), create_info.name);

        if(should_map) {
            auto locked_context = device_context.lock();
            (*locked_context)->Map(buffer->resource.Get(), 0, D3D11_MAP_WRITE, 0, &buffer->mapped_data);
        }

        buffer->size = create_info.size;
        buffer->name = create_info.name;

        return buffer;
    }

    Rx::Ptr<Image> RenderDevice::create_image(const ImageCreateInfo& create_info) const {
        auto format = to_dxgi_format(create_info.format); // TODO: Different to_dxgi_format functions for the different kinds of things
        if(format == DXGI_FORMAT_D32_FLOAT) {
            format = DXGI_FORMAT_R32_TYPELESS; // Create depth buffers with a TYPELESS format
        }
        auto desc = D3D11_TEXTURE2D_DESC{
            .Width = static_cast<Uint32>(round(create_info.width)),
            .Height = static_cast<Uint32>(round(create_info.height)),
            .Format = format,
            .Usage = D3D11_USAGE_DEFAULT,
        };

        auto image = Rx::make_ptr<Image>(RX_SYSTEM_ALLOCATOR);
        image->format = create_info.format;

        switch(create_info.usage) {
            case ImageUsage::RenderTarget:
                desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                break;

            case ImageUsage::SampledImage:
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                break;

            case ImageUsage::DepthStencil:
                desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                break;

            case ImageUsage::UnorderedAccess:
                desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
                break;
        }

        const auto result = device->CreateTexture2D(&desc, nullptr, &image->resource);
        if(FAILED(result)) {
            logger->error("Could not create image %s", create_info.name);
            return {};
        }

        image->name = create_info.name;
        image->width = static_cast<Uint32>(desc.Width);
        image->height = desc.Height;

        set_object_name(image->resource.Get(), create_info.name);

        return image;
    }

    ComPtr<ID3D11RenderTargetView> RenderDevice::create_rtv(const Image& image) const {
        ComPtr<ID3D11RenderTargetView> rtv;

        const auto result = device->CreateRenderTargetView(image.resource.Get(), nullptr, &rtv);
        if(FAILED(result)) {
            logger->error("Could not create RTV for image %s", image.name);
            return {};
        }

        return rtv;
    }

    ComPtr<ID3D11DepthStencilView> RenderDevice::create_dsv(const Image& image) const {
        ComPtr<ID3D11DepthStencilView> dsv;

        const auto result = device->CreateDepthStencilView(image.resource.Get(), nullptr, &dsv);
        if(FAILED(result)) {
            logger->error("Could not create DSV for image %s", image.name);
            return {};
        }

        return dsv;
    }

    ComPtr<ID3D11RenderTargetView> RenderDevice::get_backbuffer_rtv() const {
        ComPtr<ID3D11Texture2D> backbuffer;
        auto result = swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
        if(FAILED(result)) {
            logger->error("Could not get backbuffer image");
            return {};
        }

        ComPtr<ID3D11RenderTargetView> rtv;
        result = device->CreateRenderTargetView(backbuffer.Get(), nullptr, &rtv);
        if(FAILED(result)) {
            logger->error("Could not create RTV for backbuffer image");
            return {};
        }

        return rtv;
    }

    Vec2u RenderDevice::get_backbuffer_size() const {
        DXGI_SWAP_CHAIN_DESC desc;
        swapchain->GetDesc(&desc);

        return {desc.BufferDesc.Width, desc.BufferDesc.Height};
    }

    bool RenderDevice::map_buffer(Buffer& buffer) {
        auto locked_context = device_context.lock();
        const auto result = (*locked_context)->Map(buffer.resource.Get(), 0, D3D11_MAP_WRITE, 0, &buffer.mapped_data);
        if(FAILED(result)) {
            logger->error("Could not map buffer: %s", to_string(result));
            return false;
        }

        return true;
    }

    DescriptorType to_descriptor_type(const D3D_SHADER_INPUT_TYPE type) {
        switch(type) {
            case D3D_SIT_CBUFFER:
                return DescriptorType::ConstantBuffer;

            case D3D_SIT_TBUFFER: // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case D3D_SIT_TEXTURE:
                [[fallthrough]];
            case D3D_SIT_STRUCTURED:
                return DescriptorType::ShaderResource;

            case D3D_SIT_UAV_RWTYPED:
                [[fallthrough]];
            case D3D_SIT_UAV_RWSTRUCTURED:
                return DescriptorType::UnorderedAccess;

            case D3D_SIT_SAMPLER: // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case D3D_SIT_BYTEADDRESS:
                [[fallthrough]];
            case D3D_SIT_UAV_RWBYTEADDRESS:
                [[fallthrough]];
            case D3D_SIT_UAV_APPEND_STRUCTURED:
                [[fallthrough]];
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
                [[fallthrough]];
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                [[fallthrough]];
            case D3D_SIT_RTACCELERATIONSTRUCTURE:
                [[fallthrough]];
            case D3D_SIT_UAV_FEEDBACKTEXTURE:
                [[fallthrough]];
            default:
                logger->error("Unknown descriptor type, defaulting to UAV");
                return DescriptorType::UnorderedAccess;
        }
    }

    SynchronizedResourceAccessor<ComPtr<ID3D11DeviceContext>> RenderDevice::get_device_context() { return device_context.lock(); }

    void RenderDevice::begin_frame() {
        ZoneScoped;

        cur_swapchain_idx = swapchain->GetCurrentBackBufferIndex();

        in_init_phase = false;
    }

    void RenderDevice::end_frame() const {
        ZoneScoped;

        const auto result = swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        if(result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
            logger->error("Device lost on present :(");
        }
    }

    void RenderDevice::begin_capture() const {
        if(graphics_analysis) {
            graphics_analysis->BeginCapture();
        }
    }

    void RenderDevice::end_capture() const {
        if(graphics_analysis) {
            graphics_analysis->EndCapture();
        }
    }

    Uint32 RenderDevice::get_max_num_gpu_frames() const { return static_cast<Uint32>(cvar_max_in_flight_gpu_frames->get()); }

    bool RenderDevice::has_separate_device_memory() const { return !is_uma; }

    Buffer RenderDevice::get_staging_buffer(const Uint32 num_bytes) {
        ZoneScoped;

        for(size_t i = 0; i < staging_buffers.size(); i++) {
            if(staging_buffers[i].size >= num_bytes) {
                // Return the first suitable buffer we find
                auto buffer = std::move(staging_buffers[i]);
                staging_buffers.erase(i, i + 1);

                return buffer;
            }
        }

        // No suitable buffer is available, let's make a new one
        return create_staging_buffer(num_bytes);
    }

    void RenderDevice::return_staging_buffer(Buffer&& buffer) { staging_buffers.push_back(Rx::Utility::move(buffer)); }

    ID3D11Device* RenderDevice::get_d3d11_device() const { return device.Get(); }

    void RenderDevice::enable_debugging() {
        const auto result = device->QueryInterface(IID_PPV_ARGS(&info_queue));
        if(SUCCEEDED(result)) {
            if(cvar_break_on_validation_error->get()) {
                info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, 1);
                info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, 1);
            }
        } else {
            logger->error("Could not get the D3D11 info queue");
        }
    }

    void RenderDevice::initialize_dxgi() {
        ZoneScoped;

        ComPtr<IDXGIFactory> basic_factory;
        auto result = CreateDXGIFactory(IID_PPV_ARGS(&basic_factory));
        if(FAILED(result)) {
            Rx::abort("Could not initialize DXGI");
        }

        result = basic_factory->QueryInterface(factory.GetAddressOf());
        if(FAILED(result)) {
            Rx::abort("DXGI is not at a new enough version, please update your graphics drivers");
        }
    }

    void RenderDevice::create_device_and_swapchain(const glm::uvec2& window_size) {
        ZoneScoped;

        const auto swapchain_desc = DXGI_SWAP_CHAIN_DESC{
            .BufferDesc =
                {
                    .Width = static_cast<UINT>(window_size.x),
                    .Height = static_cast<UINT>(window_size.y),
                    .Format = swapchain_format,
                },
            .SampleDesc = {1, 0},

            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = static_cast<Uint32>(cvar_max_in_flight_gpu_frames->get()),

            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
        };

        auto locked_context = device_context.lock();

        const auto feature_level = D3D_FEATURE_LEVEL_11_1;

        ComPtr<IDXGISwapChain> basic_swapchain;
        auto hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                                D3D_DRIVER_TYPE_HARDWARE,
                                                nullptr,
#ifndef NDEBUG
                                                D3D11_CREATE_DEVICE_DEBUG,
#else
                                                0,
#endif
                                                &feature_level,
                                                1,
                                                D3D11_SDK_VERSION,
                                                &swapchain_desc,
                                                &basic_swapchain,
                                                &device,
                                                nullptr,
                                                &(*locked_context));
        if(FAILED(hr)) {
            const auto msg = Rx::String::format("Could not create device and swapchain: %s", to_string(hr));
            Rx::abort(msg.data());
        }

        hr = basic_swapchain->QueryInterface(swapchain.ReleaseAndGetAddressOf());
        if(FAILED(hr)) {
            const auto msg = Rx::String::format("Could not create device and swapchain: %s", to_string(hr));
            Rx::abort(msg.data());
        }
    }

    Buffer RenderDevice::create_staging_buffer(const Uint32 num_bytes) {
        auto desc = D3D11_BUFFER_DESC{
            .ByteWidth = num_bytes,
            .Usage = D3D11_USAGE_STAGING,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        auto buffer = Buffer{};
        auto result = device->CreateBuffer(&desc, nullptr, &buffer.resource);
        if(FAILED(result)) {
            const auto msg = Rx::String::format("Could not create staging buffer: %s (%u)", to_string(result), result);
            Rx::abort(msg.data());
        }

        {
            auto locked_context = device_context.lock();

            result = (*locked_context)->Map(buffer.resource.Get(), 0, D3D11_MAP_WRITE, 0, &buffer.mapped_data);
            if(FAILED(result)) {
                const auto msg = Rx::String::format("Could not map staging buffer: %s (%u)", to_string(result), result);
                Rx::abort(msg.data());
            }
        }

        const auto msg = Rx::String::format("Staging Buffer %d", staging_buffer_idx);
        set_object_name(buffer.resource.Get(), msg);
        staging_buffer_idx++;

        return buffer;
    }

    Rx::Ptr<RenderDevice> make_render_device(GLFWwindow* window, const Settings& settings) {
        auto hwnd = glfwGetWin32Window(window);

        glm::ivec2 framebuffer_size{};
        glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);

        logger->info("Creating D3D12 backend");

        return Rx::make_ptr<RenderDevice>(RX_SYSTEM_ALLOCATOR, hwnd, framebuffer_size, settings);
    }
} // namespace renderer

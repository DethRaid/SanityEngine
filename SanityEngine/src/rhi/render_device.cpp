#include "render_device.hpp"

#include <DirectXMath.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <spdlog/spdlog.h>

#include "d3d12/d3d12_render_device.hpp"

namespace rhi {
    std::unique_ptr<RenderDevice> make_render_device(const RenderBackend backend, GLFWwindow* window, const uint32_t num_frames) {
        switch(backend) {
            case RenderBackend::D3D12: {
                const auto hwnd = glfwGetWin32Window(window);

                XMINT2 framebuffer_size{};
                glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);

                spdlog::info("Creating D3D12 backend");

                return std::make_unique<D3D12RenderDevice>(hwnd, framebuffer_size, num_frames);
            }
        }

        spdlog::error("Unrecognized render backend type");

        return {};
    }
} // namespace rhi

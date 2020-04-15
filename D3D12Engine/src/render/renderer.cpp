#include "renderer.hpp"

#include <DirectXMath.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "d3d12/d3d12_render_device.hpp"

namespace render {
    std::unique_ptr<RenderDevice> make_render_device(const RenderBackend backend, GLFWwindow* window) {
        switch(backend) {
            case RenderBackend::D3D12: {
                const auto hwnd = glfwGetWin32Window(window);

                XMINT2 framebuffer_size;
                glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);

                return std::make_unique<D3D12RenderDevice>(hwnd, framebuffer_size);
            }
        }

        return {};
    }
} // namespace render

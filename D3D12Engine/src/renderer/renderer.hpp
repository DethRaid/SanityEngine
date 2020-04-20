#pragma once

namespace rhi {
    class RenderDevice;
}

struct GLFWwindow;

namespace renderer {
    /*!
     * \brief Renderer class that uses a clustered forward lighting algorithm
     *
     * It won't actually do that for a while, but having a strong name is very useful
     */
    class ClusteredForwardRenderer {
    public:
        explicit ClusteredForwardRenderer(rhi::RenderDevice* device, GLFWwindow* window);

        void render_scene();
    };
} // namespace renderer

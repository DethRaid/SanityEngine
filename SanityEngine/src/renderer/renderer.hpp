#pragma once

#include <entt/fwd.hpp>

#include "../rhi/mesh_data_store.hpp"
#include "../rhi/render_pipeline_state.hpp"
#include "components.hpp"

namespace rhi {
    class RenderCommandList;
    class RenderDevice;
} // namespace rhi

struct GLFWwindow;

namespace renderer {
    /*!
     * \brief Renderer class that uses a clustered forward lighting algorithm
     *
     * It won't actually do that for a while, but having a strong name is very useful
     */
    class Renderer {
    public:
        explicit Renderer(GLFWwindow* window);

        void render_scene(entt::registry& registry);

        [[nodiscard]] StaticMeshRenderable create_static_mesh(const std::vector<BveVertex>& vertices,
                                                              const std::vector<uint32_t>& indices) const;

    private:
        std::unique_ptr<rhi::RenderDevice> render_device;

        std::unique_ptr<rhi::MeshDataStore> static_mesh_storage;

        std::unique_ptr<rhi::RenderPipelineState> debug_pipeline;

#pragma region Initialization
        void make_static_mesh_storage();

        void create_debug_pipeline();
#pragma endregion

#pragma region 3D Scene
        void render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list) const;
#pragma endregion
    };
} // namespace renderer

#pragma once

#include <entt/fwd.hpp>

#include "../rhi/mesh_data_store.hpp"
#include "../rhi/render_pipeline_state.hpp"
#include "../settings.hpp"
#include "camera_matrix_buffer.hpp"
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
        explicit Renderer(GLFWwindow* window, const Settings& settings);

        void begin_frame();

        void render_scene(entt::registry& registry) const;

        void end_frame();

        [[nodiscard]] StaticMeshRenderableComponent create_static_mesh(const std::vector<BveVertex>& vertices,
                                                                       const std::vector<uint32_t>& indices) const;

    private:
        std::unique_ptr<rhi::RenderDevice> render_device;

        std::unique_ptr<rhi::MeshDataStore> static_mesh_storage;

        std::unique_ptr<rhi::RenderPipelineState> debug_pipeline;

        std::unique_ptr<CameraMatrixBuffer> camera_matrix_buffers;

#pragma region Initialization
        void make_static_mesh_storage();

        void create_debug_pipeline();
#pragma endregion

        void update_cameras(entt::registry& registry, rhi::RenderCommandList& commands, uint32_t frame_idx) const;

#pragma region 3D Scene
        void render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list, uint32_t frame_idx) const;
#pragma endregion
    };
} // namespace renderer

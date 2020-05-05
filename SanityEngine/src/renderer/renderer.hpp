#pragma once

#include <entt/fwd.hpp>

#include "../rhi/mesh_data_store.hpp"
#include "../rhi/render_pipeline_state.hpp"
#include "../settings.hpp"
#include "camera_matrix_buffer.hpp"
#include "components.hpp"
#include "material_data_buffer.hpp"

namespace rhi {
    class RenderCommandList;
    class RenderDevice;
} // namespace rhi

struct GLFWwindow;

namespace renderer {
    struct TextureHandle {
        uint32_t handle;
    };

    /*!
     * \brief Renderer class that uses a clustered forward lighting algorithm
     *
     * It won't actually do that for a while, but having a strong name is very useful
     */
    class Renderer {
    public:
        explicit Renderer(GLFWwindow* window, const Settings& settings);

        void begin_frame(uint64_t frame_count);

        void render_scene(entt::registry& registry) const;

        void end_frame();

        [[nodiscard]] StaticMeshRenderableComponent create_static_mesh(const std::vector<BveVertex>& vertices,
                                                                       const std::vector<uint32_t>& indices) const;

        [[yesdiscard]] TextureHandle create_image(const rhi::ImageCreateInfo& create_info);

        [[nodiscard]] rhi::Image& get_image(const std::string& image_name) const;

    private:
        std::unique_ptr<rhi::RenderDevice> render_device;

        std::unique_ptr<rhi::MeshDataStore> static_mesh_storage;

        std::unique_ptr<rhi::RenderPipelineState> standard_pipeline;

        std::unique_ptr<CameraMatrixBuffer> camera_matrix_buffers;

        std::unique_ptr<MaterialDataBuffer> material_data_buffer;

        std::unique_ptr<rhi::Framebuffer> scene_framebuffer;

        std::unique_ptr<rhi::RenderPipelineState> backbuffer_output_pipeline;

        std::unordered_map<std::string, uint32_t> image_name_to_index;
        std::vector<std::unique_ptr<rhi::Image>> all_images;

#pragma region Initialization
        void make_static_mesh_storage();

        void create_standard_pipeline();
        
        void create_backbuffer_output_pipeline();

        void create_scene_framebuffer(glm::uvec2 size);
#pragma endregion

        [[nodiscard]] std::vector<const rhi::Image*> get_texture_array() const;

        void update_cameras(entt::registry& registry, rhi::RenderCommandList& commands, uint32_t frame_idx) const;

#pragma region 3D Scene
        void render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list, uint32_t frame_idx) const;
#pragma endregion
    };
} // namespace renderer

#pragma once

#include <entt/fwd.hpp>

#include "../rhi/mesh_data_store.hpp"
#include "../rhi/render_pipeline_state.hpp"
#include "../settings.hpp"
#include "camera_matrix_buffer.hpp"
#include "material_data_buffer.hpp"
#include "render_components.hpp"

namespace rhi {
    class RenderCommandList;
    class RenderDevice;
} // namespace rhi

struct GLFWwindow;

namespace renderer {
    struct TextureHandle {
        uint32_t handle;
    };

    struct MaterialHandle {
        uint32_t handle;
    };

    /*!
     * \brief Renderer class that uses a clustered forward lighting algorithm
     *
     * It won't actually do that for a while, but having a strong name is very useful
     */
    class Renderer {
    public:
        explicit Renderer(GLFWwindow* window, const Settings& settings_in);

        void begin_frame(uint64_t frame_count);

        void render_scene(entt::registry& registry);

        void end_frame();

        [[nodiscard]] StaticMeshRenderableComponent create_static_mesh(const std::vector<BveVertex>& vertices,
                                                                       const std::vector<uint32_t>& indices) const;

        [[yesdiscard]] TextureHandle create_image(const rhi::ImageCreateInfo& create_info);

        [[nodiscard]] rhi::Image& get_image(const std::string& image_name) const;

    private:
        Settings settings;

        std::unique_ptr<rhi::RenderDevice> render_device;

        std::unique_ptr<rhi::MeshDataStore> static_mesh_storage;

        std::unique_ptr<rhi::RenderPipelineState> standard_pipeline;

        std::unique_ptr<CameraMatrixBuffer> camera_matrix_buffers;

        std::unique_ptr<MaterialDataBuffer> material_data_buffer;
        std::vector<std::unique_ptr<rhi::Buffer>> material_device_buffers;

        std::unique_ptr<rhi::Framebuffer> scene_framebuffer;

        std::unique_ptr<rhi::RenderPipelineState> backbuffer_output_pipeline;
        MaterialHandle backbuffer_output_material;

        std::unordered_map<std::string, uint32_t> image_name_to_index;
        std::vector<std::unique_ptr<rhi::Image>> all_images;

        std::unique_ptr<rhi::Image> scene_depth_target;

        std::array<Light, MAX_NUM_LIGHTS> lights;
        std::vector<std::unique_ptr<rhi::Buffer>> light_device_buffers;

#pragma region Initialization
        void create_static_mesh_storage();

        void create_material_data_buffers();

        void create_standard_pipeline();

        void create_scene_framebuffer(glm::uvec2 size);

        void create_backbuffer_output_pipeline_and_material();

        void create_light_buffers();
#pragma endregion

        [[nodiscard]] std::vector<const rhi::Image*> get_texture_array() const;

        void update_cameras(entt::registry& registry, rhi::RenderCommandList& commands, uint32_t frame_idx) const;

#pragma region 3D Scene
        void update_lights(entt::registry& registry, uint32_t frame_idx);

        void render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list, uint32_t frame_idx);
#pragma endregion
    };
} // namespace renderer

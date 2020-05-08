#pragma once

#include <queue>

#include <entt/fwd.hpp>

#include "../rhi/mesh_data_store.hpp"
#include "../rhi/render_pipeline_state.hpp"
#include "../settings.hpp"
#include "camera_matrix_buffer.hpp"
#include "handles.hpp"
#include "material_data_buffer.hpp"
#include "render_components.hpp"

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
        static std::vector<BveVertex> cube_vertices;
        static std::vector<uint32_t> cube_indices;

        explicit Renderer(GLFWwindow* window, const Settings& settings_in);

        void begin_frame(uint64_t frame_count);

        void render_scene(entt::registry& registry);

        void end_frame();

        [[nodiscard]] StaticMeshRenderableComponent create_static_mesh(const std::vector<BveVertex>& vertices,
                                                                       const std::vector<uint32_t>& indices);

        [[yesdiscard]] ImageHandle create_image(const rhi::ImageCreateInfo& create_info);

        [[nodiscard]] ImageHandle create_image(const rhi::ImageCreateInfo& create_info, const void* image_data);

        [[nodiscard]] std::optional<ImageHandle> get_image_handle(const std::string& name);

        [[nodiscard]] rhi::Image& get_image(const std::string& image_name) const;

        [[nodiscard]] MaterialDataBuffer& get_material_data_buffer() const;

        void begin_device_capture() const;

        void end_device_capture() const;

    private:
        std::shared_ptr<spdlog::logger> logger;

        Settings settings;

        std::unique_ptr<rhi::RenderDevice> device;

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

        std::queue<Mesh> pending_raytracing_upload_meshes;
        bool raytracing_scene_dirty{false};

        std::unique_ptr<rhi::RenderPipelineState> atmospheric_sky_pipeline;

#pragma region Initialization
        void create_static_mesh_storage();

        void create_material_data_buffers();

        void create_standard_pipeline();

        void create_atmospheric_sky_pipeline();

        void create_scene_framebuffer(glm::uvec2 size);

        void create_backbuffer_output_pipeline_and_material();

        void create_light_buffers();
#pragma endregion

        [[nodiscard]] std::vector<const rhi::Image*> get_texture_array() const;

        void update_cameras(entt::registry& registry, rhi::RenderCommandList& commands, uint32_t frame_idx) const;

        void upload_material_data(rhi::ResourceCommandList& command_list, uint32_t frame_idx);

#pragma region Raytracing
        rhi::RaytracingScene raytracing_scene;

        void rebuild_raytracing_scene(entt::registry& registry, rhi::RenderCommandList& command_list);
#pragma endregion

#pragma region 3D Scene
        void update_lights(entt::registry& registry, uint32_t frame_idx);

        void render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list, uint32_t frame_idx);
#pragma endregion
    };
} // namespace renderer

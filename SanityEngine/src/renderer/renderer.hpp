#pragma once

#include <queue>

#include <entt/fwd.hpp>

#include "../rhi/bind_group.hpp"
#include "../rhi/compute_pipeline_state.hpp"
#include "../rhi/framebuffer.hpp"
#include "../rhi/mesh_data_store.hpp"
#include "../rhi/render_pipeline_state.hpp"
#include "../settings.hpp"
#include "camera_matrix_buffer.hpp"
#include "handles.hpp"
#include "material_data_buffer.hpp"
#include "render_components.hpp"
#include "renderpasses/backbuffer_output_pass.hpp"
#include "renderpasses/denoiser_pass.hpp"
#include "renderpasses/forward_pass.hpp"

namespace rhi {
    class RenderCommandList;
    class RenderDevice;
} // namespace rhi

struct GLFWwindow;

namespace renderer {
    /*!
     * \brief Data that remains constant for the entire frame
     */
    struct PerFrameData {
        /*!
         * \brief Number of seconds since the program started
         */
        float time_since_start{0};
    };

    /*!
     * \brief Renderer class that uses a clustered forward lighting algorithm
     *
     * It won't actually do that for a while, but having a strong name is very useful
     */
    class Renderer {
    public:
        static std::vector<StandardVertex> cube_vertices;
        static std::vector<uint32_t> cube_indices;

        explicit Renderer(GLFWwindow* window, const Settings& settings_in);

        void begin_frame(uint64_t frame_count);

        void render_all(entt::registry& registry);

        void end_frame() const;

        void add_raytracing_objects_to_scene(const std::vector<rhi::RaytracingObject>& new_objects);

        [[yesdiscard]] TextureHandle create_image(const rhi::ImageCreateInfo& create_info);

        [[nodiscard]] TextureHandle create_image(const rhi::ImageCreateInfo& create_info,
                                                 const void* image_data,
                                                 const ComPtr<ID3D12GraphicsCommandList4>& commands);

        [[nodiscard]] std::optional<TextureHandle> get_image_handle(const std::string& name);

        [[nodiscard]] rhi::Image& get_image(const std::string& image_name) const;

        [[nodiscard]] rhi::Image& get_image(TextureHandle handle) const;

        void schedule_texture_destruction(const TextureHandle& image_handle);

        [[nodiscard]] MaterialDataBuffer& get_material_data_buffer() const;

        [[nodiscard]] rhi::RenderDevice& get_render_device() const;

        [[nodiscard]] rhi::MeshDataStore& get_static_mesh_store() const;

        void begin_device_capture() const;

        void end_device_capture() const;

        [[nodiscard]] TextureHandle get_noise_texture() const;

        [[nodiscard]] TextureHandle get_pink_texture() const;

        [[nodiscard]] TextureHandle get_default_normal_roughness_texture() const;

        [[nodiscard]] TextureHandle get_default_specular_color_emission_texture() const;

        [[nodiscard]] RaytracableGeometryHandle create_raytracing_geometry(const rhi::Buffer& vertex_buffer,
                                                                           const rhi::Buffer& index_buffer,
                                                                           const std::vector<rhi::Mesh>& meshes,
                                                                           const ComPtr<ID3D12GraphicsCommandList4>& commands);

        [[nodiscard]] std::unique_ptr<rhi::BindGroup> bind_global_resources_for_frame(uint32_t frame_idx);

    private:
        std::shared_ptr<spdlog::logger> logger;

        std::chrono::high_resolution_clock::time_point start_time;

        Settings settings;

        glm::uvec2 output_framebuffer_size;

        std::unique_ptr<rhi::RenderDevice> device;

        std::unique_ptr<rhi::MeshDataStore> static_mesh_storage;

        PerFrameData per_frame_data;
        std::vector<std::unique_ptr<rhi::Buffer>> per_frame_data_buffers;

        std::unique_ptr<CameraMatrixBuffer> camera_matrix_buffers;

        std::unique_ptr<MaterialDataBuffer> material_data_buffer;
        std::vector<std::unique_ptr<rhi::Buffer>> material_device_buffers;

        MaterialHandle backbuffer_output_material;

        std::unordered_map<std::string, uint32_t> image_name_to_index;
        std::vector<std::unique_ptr<rhi::Image>> all_images;

        std::array<Light, MAX_NUM_LIGHTS> lights;
        std::vector<std::unique_ptr<rhi::Buffer>> light_device_buffers;

        std::queue<rhi::Mesh> pending_raytracing_upload_meshes;
        bool raytracing_scene_dirty{false};

        TextureHandle noise_texture_handle;
        TextureHandle pink_texture_handle;
        TextureHandle normal_roughness_texture_handle;
        TextureHandle specular_emission_texture_handle;

#pragma region Initialization
        void create_static_mesh_storage();

        void create_per_frame_data_buffers();

        void create_material_data_buffers();

        void create_scene_framebuffer();

        void create_accumulation_pipeline_and_material();

        void create_shadowmap_framebuffer_and_pipeline(QualityLevel quality_level);

        void create_backbuffer_output_pipeline_and_material();

        void create_light_buffers();

        void create_builtin_images();

        void load_noise_texture(const std::string& filepath);
#pragma endregion

        [[nodiscard]] std::vector<const rhi::Image*> get_texture_array() const;

        void update_cameras(entt::registry& registry, uint32_t frame_idx) const;

        void upload_material_data(uint32_t frame_idx);

#pragma region Raytracing
        std::vector<rhi::RaytracableGeometry> raytracing_geometries;

        std::vector<rhi::RaytracingObject> raytracing_objects;

        rhi::RaytracingScene raytracing_scene;

        std::unique_ptr<DenoiserPass> denoiser_pass;

        void rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands);
#pragma endregion

#pragma region 3D Scene
        std::unique_ptr<rhi::Image> shadow_map_image;
        std::unique_ptr<rhi::Framebuffer> shadow_map_framebuffer;

        uint32_t shadow_camera_index;

        std::unique_ptr<rhi::RenderPipelineState> shadow_pipeline;

        std::unique_ptr<ForwardPass> forward_pass;

        std::unique_ptr<BackbufferOutputPass> backbuffer_output_pass;

        void update_lights(entt::registry& registry, uint32_t frame_idx);

        void render_3d_scene(entt::registry& registry, ID3D12GraphicsCommandList4* commands, uint32_t frame_idx);
#pragma endregion

#pragma region UI
        std::unique_ptr<rhi::RenderPipelineState> ui_pipeline;

        std::vector<std::unique_ptr<rhi::Buffer>> ui_vertex_buffers;
        std::vector<std::unique_ptr<rhi::Buffer>> ui_index_buffers;

        void create_ui_pipeline();

        void create_ui_mesh_buffers();

        void render_ui(const ComPtr<ID3D12GraphicsCommandList4>& commands, uint32_t frame_idx) const;
#pragma endregion
    };
} // namespace renderer

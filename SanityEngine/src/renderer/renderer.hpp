#pragma once

#include <chrono>
#include <queue>

#include <entt/fwd.hpp>
#include <rx/core/ptr.h>
#include <rx/core/vector.h>

#include "renderer/camera_matrix_buffer.hpp"
#include "renderer/handles.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderpasses/backbuffer_output_pass.hpp"
#include "renderer/renderpasses/denoiser_pass.hpp"
#include "renderer/renderpasses/forward_pass.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/bind_group.hpp"
#include "rhi/compute_pipeline_state.hpp"
#include "rhi/framebuffer.hpp"
#include "rhi/mesh_data_store.hpp"
#include "rhi/raytracing_structs.hpp"
#include "rhi/render_pipeline_state.hpp"
#include "settings.hpp"

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
        static Rx::Vector<StandardVertex> cube_vertices;
        static Rx::Vector<uint32_t> cube_indices;

        explicit Renderer(GLFWwindow* window, const Settings& settings_in);

        void begin_frame(uint64_t frame_count);

        void render_all(entt::registry& registry);

        void end_frame() const;

        void add_raytracing_objects_to_scene(const Rx::Vector<rhi::RaytracingObject>& new_objects);

        [[yesdiscard]] TextureHandle create_image(const rhi::ImageCreateInfo& create_info);

        [[nodiscard]] TextureHandle create_image(const rhi::ImageCreateInfo& create_info,
                                                 const void* image_data,
                                                 const ComPtr<ID3D12GraphicsCommandList4>& commands);

        [[nodiscard]] Rx::Optional<TextureHandle> get_image_handle(const Rx::String& name);

        [[nodiscard]] rhi::Image& get_image(const Rx::String& image_name) const;

        [[nodiscard]] rhi::Image& get_image(TextureHandle handle) const;

        void schedule_texture_destruction(const TextureHandle& image_handle);

        [[nodiscard]] StandardMaterialHandle allocate_standard_material(const StandardMaterial& material);

        [[nodiscard]] rhi::Buffer& get_standard_material_buffer_for_frame(uint32_t frame_idx) const;

        void deallocate_standard_material(StandardMaterialHandle handle);

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
                                                                           const Rx::Vector<rhi::Mesh>& meshes,
                                                                           const ComPtr<ID3D12GraphicsCommandList4>& commands);

        [[nodiscard]] Rx::Ptr<rhi::BindGroup> bind_global_resources_for_frame(uint32_t frame_idx);

    private:
        std::chrono::high_resolution_clock::time_point start_time;

        Settings settings;

        glm::uvec2 output_framebuffer_size;

        Rx::Ptr<rhi::RenderDevice> device;

        Rx::Ptr<rhi::MeshDataStore> static_mesh_storage;

        PerFrameData per_frame_data;
        Rx::Vector<Rx::Ptr<rhi::Buffer>> per_frame_data_buffers;

        Rx::Ptr<CameraMatrixBuffer> camera_matrix_buffers;

        Rx::Vector<StandardMaterial> standard_materials;
        Rx::Vector<StandardMaterialHandle> free_material_handles;
        Rx::Vector<Rx::Ptr<rhi::Buffer>> material_device_buffers;

        Rx::Map<Rx::String, uint32_t> image_name_to_index;
        Rx::Vector<Rx::Ptr<rhi::Image>> all_images;

        std::array<Light, MAX_NUM_LIGHTS> lights;
        Rx::Vector<Rx::Ptr<rhi::Buffer>> light_device_buffers;

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

        void create_light_buffers();

        void create_builtin_images();

        void load_noise_texture(const Rx::String& filepath);
#pragma endregion

        [[nodiscard]] Rx::Vector<const rhi::Image*> get_texture_array() const;

        void update_cameras(entt::registry& registry, uint32_t frame_idx) const;

        void upload_material_data(uint32_t frame_idx);

#pragma region 3D Scene
        Rx::Vector<rhi::RaytracableGeometry> raytracing_geometries;

        Rx::Vector<rhi::RaytracingObject> raytracing_objects;

        rhi::RaytracingScene raytracing_scene;

        Rx::Ptr<ForwardPass> forward_pass;

        Rx::Ptr<DenoiserPass> denoiser_pass;

        Rx::Ptr<BackbufferOutputPass> backbuffer_output_pass;

        void rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands);

        void update_lights(entt::registry& registry, uint32_t frame_idx);

        void render_3d_scene(entt::registry& registry, ID3D12GraphicsCommandList4* commands, uint32_t frame_idx);
#pragma endregion

#pragma region UI
        Rx::Ptr<rhi::RenderPipelineState> ui_pipeline;

        Rx::Vector<Rx::Ptr<rhi::Buffer>> ui_vertex_buffers;
        Rx::Vector<Rx::Ptr<rhi::Buffer>> ui_index_buffers;

        void create_ui_pipeline();

        void create_ui_mesh_buffers();

        void render_ui(const ComPtr<ID3D12GraphicsCommandList4>& commands, uint32_t frame_idx) const;
#pragma endregion
    };
} // namespace renderer

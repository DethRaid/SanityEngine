#pragma once

#include <chrono>
#include <queue>

#include "core/Prelude.hpp"
#include "core/VectorHandle.hpp"
#include "core/async/synchronized_resource.hpp"
#include "renderer/camera_matrix_buffer.hpp"
#include "renderer/handles.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderpasses/RaytracedLightingPass.hpp"
#include "renderer/renderpasses/denoiser_pass.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/bind_group.hpp"
#include "rhi/mesh_data_store.hpp"
#include "rhi/raytracing_structs.hpp"
#include "rhi/render_device.hpp"
#include "rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"
#include "rx/core/vector.h"
#include "settings.hpp"
#include "single_pass_downsampler.hpp"

namespace std {
    namespace filesystem {
        class path;
    }
} // namespace std

using Microsoft::WRL::ComPtr;

struct GLFWwindow;

namespace sanity::engine::renderer {
    class RenderCommandList;

    using RenderpassHandle = VectorHandle<Rx::Ptr<RenderPass>>;

    /*!
     * \brief All the information needed to decide whether or not to issue a drawcall for an object
     */
    struct VisibleObjectCullingInformation {
        /*!
         * \brief Min and max of this object's bounding box, along the x axis
         */
        Vec2f aabb_x_min_max{};

        /*!
         * \brief Min and max of this object's bounding box, along the y axis
         */
        Vec2f aabb_y_min_max{};

        /*!
         * \brief Min and max of this object's bounding box, along the z axis
         */
        Vec2f aabb_z_min_max{};

        Mat4x4f model_matrix{};

        Uint32 vertex_count{};

        Uint32 start_vertex_location{};
    };

    /*!
     * \brief Data that remains constant for the entire frame
     */
    struct PerFrameData {
        /*!
         * \brief Number of seconds since the program started
         */
        Float32 time_since_start{0};
    };

    /*!
     * \brief Renderer class that uses a clustered forward lighting algorithm
     *
     * It won't actually do that for a while, but having a strong name is very useful
     */
    class Renderer {
    public:
        inline static const TextureHandle BACKBUFFER_HANDLE{0xF0000000};

        explicit Renderer(GLFWwindow* window);

        /// <summary>
        /// Reloads all the shaders from disk
        /// </summary>
        void reload_shaders();

        void begin_frame(uint64_t frame_count);

        void render_frame(SynchronizedResourceAccessor<entt::registry>& registryd);

        void end_frame() const;

        void add_raytracing_objects_to_scene(const Rx::Vector<RaytracingObject>& new_objects);

        TextureHandle create_image(const ImageCreateInfo& create_info);

        [[nodiscard]] TextureHandle create_image(const ImageCreateInfo& create_info,
                                                 const void* image_data,
                                                 ID3D12GraphicsCommandList4* commands);
        
        [[nodiscard]] Rx::Optional<TextureHandle> get_image_handle(const Rx::String& name);

        [[nodiscard]] Image get_image(const Rx::String& image_name) const;

        [[nodiscard]] Image get_image(TextureHandle handle) const;

        void schedule_texture_destruction(const TextureHandle& image_handle);

        /*!
         * \brief Sets the image that the 3D scene will be rendered to
         */
        void set_scene_output_image(TextureHandle output_image_handle);

        [[nodiscard]] StandardMaterialHandle allocate_standard_material(const StandardMaterial& material);

        [[nodiscard]] Buffer& get_standard_material_buffer_for_frame(Uint32 frame_idx) const;

        void deallocate_standard_material(StandardMaterialHandle handle);

        [[nodiscard]] RenderBackend& get_render_backend() const;

        [[nodiscard]] MeshDataStore& get_static_mesh_store() const;

        void begin_device_capture() const;

        void end_device_capture() const;

        [[nodiscard]] TextureHandle get_noise_texture() const;

        [[nodiscard]] TextureHandle get_pink_texture() const;

        [[nodiscard]] TextureHandle get_default_normal_texture() const;

        [[nodiscard]] TextureHandle get_default_metallic_roughness_texture() const;

        [[nodiscard]] RaytracingASHandle create_raytracing_geometry(const Buffer& vertex_buffer,
                                                                    const Buffer& index_buffer,
                                                                    const Rx::Vector<PlacedMesh>& meshes,
                                                                    ID3D12GraphicsCommandList4* commands);

        [[nodiscard]] Rx::Ptr<BindGroup> bind_global_resources_for_frame(Uint32 frame_idx);

        [[nodiscard]] Buffer& get_model_matrix_for_frame(Uint32 frame_idx);

        Uint32 add_model_matrix_to_frame(const glm::mat4& model_matrix, Uint32 frame_idx);
    	
        [[nodiscard]] SinglePassDownsampler& get_spd();

    private:
        std::chrono::high_resolution_clock::time_point start_time;

        glm::uvec2 output_framebuffer_size;

        Rx::Ptr<RenderBackend> device;

        Rx::Ptr<MeshDataStore> static_mesh_storage;

        PerFrameData per_frame_data;
        Rx::Vector<Rx::Ptr<Buffer>> per_frame_data_buffers;

        Rx::Ptr<CameraMatrixBuffer> camera_matrix_buffers;

        Rx::Vector<StandardMaterial> standard_materials;
        Rx::Vector<StandardMaterialHandle> free_material_handles;
        Rx::Vector<Rx::Ptr<Buffer>> material_device_buffers;

        Rx::Map<Rx::String, Uint32> image_name_to_index;
        Rx::Vector<Rx::Ptr<Image>> all_images;
        Rx::Ptr<SinglePassDownsampler> spd;

        Rx::Array<Light[MAX_NUM_LIGHTS]> lights;
        Rx::Vector<Rx::Ptr<Buffer>> light_device_buffers;

        std::queue<Mesh> pending_raytracing_upload_meshes;
        bool raytracing_scene_dirty{false};

        TextureHandle noise_texture_handle;
        TextureHandle pink_texture_handle;
        TextureHandle normal_roughness_texture_handle;
        TextureHandle specular_emission_texture_handle;

        Rx::Vector<Rx::Ptr<RenderPass>> render_passes;

        RenderpassHandle forward_pass_handle;
        RenderpassHandle denoiser_pass_handle;
        RenderpassHandle scene_output_pass_handle;
        RenderpassHandle imgui_pass_handle;

        ComPtr<ID3D12PipelineState> single_pass_denoiser_pipeline;

#pragma region Initialization
        void create_static_mesh_storage();

        void create_per_frame_buffers();

        void create_material_data_buffers();

        void create_light_buffers();

        void create_builtin_images();

        void load_noise_texture(const std::filesystem::path& filepath);

        void create_render_passes();

        void reload_builtin_shaders();

        void reload_renderpass_shaders();
#pragma endregion

        [[nodiscard]] Rx::Vector<const Image*> get_texture_array() const;

        void update_cameras(entt::registry& registry, Uint32 frame_idx) const;

        void upload_material_data(Uint32 frame_idx);

#pragma region Renderpasses
        void execute_all_render_passes(ComPtr<ID3D12GraphicsCommandList4>& command_list,
                                       SynchronizedResourceAccessor<entt::registry>& registry,
                                       const Uint32& frame_idx);

        void issue_pre_pass_barriers(ID3D12GraphicsCommandList* command_list,
                                     Uint32 render_pass_index,
                                     const Rx::Ptr<RenderPass>& render_pass);

        void issue_post_pass_barriers(ID3D12GraphicsCommandList* command_list,
                                      Uint32 render_pass_index,
                                      const Rx::Ptr<RenderPass>& render_pass);

        [[nodiscard]] Rx::Map<TextureHandle, D3D12_RESOURCE_STATES> get_previous_resource_states(Uint32 cur_renderpass_index) const;

        [[nodiscard]] Rx::Map<TextureHandle, D3D12_RESOURCE_STATES> get_next_resource_states(Uint32 cur_renderpass_index) const;
#pragma endregion

#pragma region 3D Scene
        Rx::Vector<RaytracingAccelerationStructure> raytracing_geometries;

        Rx::Vector<RaytracingObject> raytracing_objects;

        Rx::Vector<Rx::Ptr<Buffer>> model_matrix_buffers;

        Rx::Vector<Rx::Ptr<Rx::Concurrency::Atomic<Uint32>>> next_unused_model_matrix_per_frame;

        RaytracingScene raytracing_scene;

        Rx::Ptr<Buffer> visible_objects_buffer;

        void rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands);

        void update_lights(entt::registry& registry, Uint32 frame_idx);
#pragma endregion
    };
} // namespace sanity::engine::renderer

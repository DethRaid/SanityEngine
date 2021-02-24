#pragma once

#include <chrono>
#include <queue>

#include "core/Prelude.hpp"
#include "core/VectorHandle.hpp"
#include "renderer/camera_matrix_buffer.hpp"
#include "renderer/handles.hpp"
#include "renderer/hlsl/shared_structs.hpp"
#include "renderer/hlsl/standard_material.hpp"
#include "renderer/mesh_data_store.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderpasses/ObjectsPass.hpp"
#include "renderer/renderpasses/denoiser_pass.hpp"
#include "renderer/rhi/raytracing_structs.hpp"
#include "renderer/rhi/render_backend.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "renderer/single_pass_downsampler.hpp"
#include "rx/core/ptr.h"
#include "rx/core/vector.h"
#include "settings.hpp"

namespace std {
    namespace filesystem {
        class path;
    }
} // namespace std

using Microsoft::WRL::ComPtr;

struct GLFWwindow;

namespace sanity::engine::renderer {
    class DearImGuiRenderPass;
    class PostprocessingPass;
    class RenderCommandList;

    template <typename RenderPassType>
    class RenderpassHandle : public VectorHandle<Rx::Ptr<RenderPass>> {
    public:
        /**
         * @brief Makes a handle to the last render pass in the container
         */
        [[nodiscard]] static RenderpassHandle<RenderPassType> make_from_last_element(Rx::Vector<Rx::Ptr<RenderPass>>& container);

        explicit RenderpassHandle(Rx::Vector<Rx::Ptr<RenderPass>>* container_in, Size index_in);

        RenderPassType* get();

        RenderPassType* get() const;
    };

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

        void render_frame(entt::registry& registry);

        void end_frame() const;

        void add_raytracing_objects_to_scene(const Rx::Vector<RaytracingObject>& new_objects);

        [[nodiscard]] BufferHandle create_buffer(const BufferCreateInfo& create_info);

        [[nodiscard]] BufferHandle create_buffer(const BufferCreateInfo& create_info, const void* data, ID3D12GraphicsCommandList2* cmds);

        [[nodiscard]] Rx::Optional<Buffer> get_buffer(const Rx::String& name) const;

        [[nodiscard]] Rx::Optional<Buffer> get_buffer(const BufferHandle& handle) const;

        template <typename DataType>
        void copy_data_to_buffer(BufferHandle handle, const Rx::Vector<DataType>& data);

        [[nodiscard]] TextureHandle create_texture(const TextureCreateInfo& create_info);

        [[nodiscard]] TextureHandle create_texture(const TextureCreateInfo& create_info,
                                                   const void* image_data,
                                                   ID3D12GraphicsCommandList2* commands,
                                                   bool generate_mipmaps = true);

        [[nodiscard]] Rx::Optional<TextureHandle> get_texture_handle(const Rx::String& name);

        [[nodiscard]] Texture get_texture(const Rx::String& name) const;

        [[nodiscard]] Texture get_texture(TextureHandle handle) const;

        void schedule_texture_destruction(const TextureHandle& texture_handle);

        [[nodiscard]] FluidVolumeHandle create_fluid_volume(const FluidVolumeCreateInfo& create_info);

        [[nodiscard]] FluidVolume& get_fluid_volume(const FluidVolumeHandle& handle);

        /*!
         * \brief Sets the image that the 3D scene will be rendered to
         */
        void set_scene_output_texture(TextureHandle output_texture_handle);

        /*!
         * \brief Gets the handle to the texture that the main scene view is rendered to
         */
        [[nodiscard]] TextureHandle get_scene_output_texture() const;

        [[nodiscard]] StandardMaterialHandle allocate_standard_material(const StandardMaterial& material);

        [[nodiscard]] const BufferHandle& get_standard_material_buffer_for_frame(Uint32 frame_idx) const;

        void deallocate_standard_material(StandardMaterialHandle handle);

        [[nodiscard]] LightHandle next_next_free_light_handle();

        void return_light_handle(LightHandle handle);

        [[nodiscard]] RaytracingAsHandle create_raytracing_geometry(const Buffer& vertex_buffer,
                                                                    const Buffer& index_buffer,
                                                                    const Rx::Vector<PlacedMesh>& meshes,
                                                                    ID3D12GraphicsCommandList4* commands);

        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE get_resource_array_gpu_descriptor(Uint32 frame_idx) const;

        [[nodiscard]] BufferHandle& get_model_matrix_for_frame(Uint32 frame_idx);

        [[nodiscard]] Uint32 add_model_matrix_to_frame(const glm::mat4& model_matrix, Uint32 frame_idx);

        void begin_device_capture() const;

        void end_device_capture() const;

        [[nodiscard]] RenderBackend& get_render_backend() const;

        [[nodiscard]] MeshDataStore& get_static_mesh_store() const;

        [[nodiscard]] SinglePassDownsampler& get_spd() const;

        [[nodiscard]] const Rx::Vector<Texture>& get_texture_array() const;

        [[nodiscard]] TextureHandle get_noise_texture() const;

        [[nodiscard]] TextureHandle get_pink_texture() const;

        [[nodiscard]] TextureHandle get_default_normal_texture() const;

        [[nodiscard]] TextureHandle get_default_metallic_roughness_texture() const;

        [[nodiscard]] BufferHandle get_frame_constants_buffer(Uint32 frame_idx) const;

        [[nodiscard]] const RaytracingScene& get_raytracing_scene() const;

    private:
        std::chrono::high_resolution_clock::time_point start_time;

        glm::uvec2 output_framebuffer_size{0, 0};

        Rx::Ptr<RenderBackend> backend;

        Rx::Ptr<MeshDataStore> static_mesh_storage;

        Rx::Map<Rx::String, BufferHandle> buffer_name_to_handle;
        Rx::Vector<Buffer> all_buffers;

        Rx::Map<Rx::String, TextureHandle> texture_name_to_index;
        Rx::Vector<Texture> all_textures;

        Rx::Map<Rx::String, FluidVolumeHandle> fluid_volume_name_to_handle;
        Rx::Vector<FluidVolume> all_fluid_volumes;

        FrameConstants frame_constants;
        Rx::Vector<BufferHandle> frame_constants_buffers;

        Rx::Ptr<CameraMatrixBuffer> camera_matrix_buffers;

        Rx::Vector<StandardMaterial> standard_materials;
        Rx::Vector<StandardMaterialHandle> free_material_handles;

        Rx::Vector<BufferHandle> material_device_buffers;

        Rx::Ptr<SinglePassDownsampler> spd;

        Uint32 next_free_light_index{1}; // Index 0 is the sun, its hardcoded and timing-dependent and all the things we hate
        Rx::Vector<LightHandle> available_light_handles;
        Rx::Vector<GpuLight> lights;
        Rx::Vector<BufferHandle> light_device_buffers;

        std::queue<Mesh> pending_raytracing_upload_meshes;
        bool raytracing_scene_dirty{false};

        TextureHandle noise_texture_handle;
        TextureHandle pink_texture_handle;
        TextureHandle normal_roughness_texture_handle;
        TextureHandle specular_emission_texture_handle;

        Rx::Vector<Rx::Ptr<RenderPass>> render_passes;

        RenderpassHandle<ObjectsPass> forward_pass_handle;
        RenderpassHandle<DenoiserPass> denoiser_pass_handle;
        RenderpassHandle<PostprocessingPass> postprocessing_pass_handle;
        RenderpassHandle<DearImGuiRenderPass> imgui_pass_handle;

        ComPtr<ID3D12PipelineState> single_pass_denoiser_pipeline;
        Rx::Vector<DescriptorRange> resource_descriptors;

#pragma region Initialization
        void create_static_mesh_storage();

        void allocate_resource_descriptors();

        void create_per_frame_buffers();

        void create_material_data_buffers();

        void create_light_buffers();

        void create_builtin_images();

        void load_noise_texture(const std::filesystem::path& filepath);

        void create_render_passes();

        void reload_builtin_shaders();

        void reload_renderpass_shaders();
#pragma endregion

        void update_cameras(entt::registry& registry, Uint32 frame_idx) const;

        void upload_material_data(Uint32 frame_idx);

#pragma region Renderpasses
        void update_resource_array_descriptors(ID3D12GraphicsCommandList* cmds, Uint32 frame_idx);

        void execute_all_render_passes(ComPtr<ID3D12GraphicsCommandList4>& command_list, entt::registry& registry, const Uint32& frame_idx);

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

        Rx::Vector<BufferHandle> model_matrix_buffers;

        Rx::Vector<Rx::Ptr<Rx::Concurrency::Atomic<Uint32>>> next_unused_model_matrix_per_frame;

        bool has_raytracing_scene{false};
        RaytracingScene raytracing_scene;

        Rx::Ptr<BufferHandle> visible_objects_buffer;

        void rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands);

        void update_light_data_buffer(entt::registry& registry, Uint32 frame_idx);

        void update_frame_constants(entt::registry& registry, Uint32 frame_idx);
#pragma endregion
    };

    template <typename RenderPassType>
    RenderpassHandle<RenderPassType> RenderpassHandle<RenderPassType>::make_from_last_element(Rx::Vector<Rx::Ptr<RenderPass>>& container) {
        return RenderpassHandle<RenderPassType>{&container, container.size() - 1};
    }

    template <typename RenderPassType>
    RenderpassHandle<RenderPassType>::RenderpassHandle(Rx::Vector<Rx::Ptr<RenderPass>>* container_in, const Size index_in)
        : VectorHandle(container_in, index_in) {}

    template <typename RenderPassType>
    RenderPassType* RenderpassHandle<RenderPassType>::get() {
        auto* renderpass = this->operator->()->get();
        return static_cast<RenderPassType*>(renderpass);
    }

    template <typename RenderPassType>
    RenderPassType* RenderpassHandle<RenderPassType>::get() const {
        auto* renderpass = this->operator->()->get();
        return static_cast<RenderPassType*>(renderpass);
    }

    template <typename DataType>
    void Renderer::copy_data_to_buffer(const BufferHandle handle, const Rx::Vector<DataType>& data) {
        const auto& buffer = get_buffer(handle);

        memcpy(buffer->mapped_ptr, data.data(), data.size() * sizeof(DataType));
    }
} // namespace sanity::engine::renderer

#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <entt/entity/registry.hpp>
#include <minitrace.h>
#include <spdlog/spdlog.h>

#include "../core/components.hpp"
#include "../core/constants.hpp"
#include "../core/ensure.hpp"
#include "../rhi/render_device.hpp"
#include "camera_matrix_buffer.hpp"
#include "components.hpp"

namespace renderer {
    constexpr const char* SCENE_COLOR_RENDER_TARGET = "Scene color target";
    constexpr const char* SCENE_DEPTH_TARGET = "Scene depth target";

    constexpr uint32_t MATERIAL_DATA_BUFFER_SIZE = 1 << 20;

    struct BackbufferOutputMaterial {
        uint32_t scene_output_index;
    };

    std::vector<uint8_t> load_shader(const std::string& shader_filename) {
        MTR_SCOPE("Renderer", "load_shader");
        auto* shader_file = fopen(shader_filename.c_str(), "rb");
        if(shader_file == nullptr) {
            spdlog::error("Could not open shader file '{}'", shader_filename);
            return {};
        }

        fseek(shader_file, 0, SEEK_END);
        const auto file_size = ftell(shader_file);

        auto shader = std::vector<uint8_t>(file_size);

        rewind(shader_file);

        fread(shader.data(), sizeof(uint8_t), file_size, shader_file);
        fclose(shader_file);

        return shader;
    }

    Renderer::Renderer(GLFWwindow* window, const Settings& settings_in)
        : settings{settings_in},
          render_device{make_render_device(rhi::RenderBackend::D3D12, window, settings_in)},
          camera_matrix_buffers{std::make_unique<CameraMatrixBuffer>(*render_device, settings_in.num_in_flight_frames)} {
        MTR_SCOPE("Renderer", "Renderer");
        create_static_mesh_storage();

        create_material_data_buffers();

        create_standard_pipeline();

        ENSURE(settings_in.render_scale > 0, "Render scale may not be 0 or less");

        int framebuffer_width;
        int framebuffer_height;
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        create_scene_framebuffer(glm::uvec2{framebuffer_width * settings_in.render_scale, framebuffer_height * settings_in.render_scale});

        create_backbuffer_output_pipeline_and_material();
    }

    void Renderer::begin_frame(const uint64_t frame_count) { render_device->begin_frame(frame_count); }

    void Renderer::render_scene(entt::registry& registry) const {
        MTR_SCOPE("Renderer", "render_scene");

        const auto frame_idx = render_device->get_cur_gpu_frame_idx();

        auto command_list = render_device->create_render_command_list();
        command_list->set_debug_name("Main Render Command List");

        update_cameras(registry, *command_list, frame_idx);

        render_3d_scene(registry, *command_list, frame_idx);

        render_device->submit_command_list(std::move(command_list));
    }

    void Renderer::end_frame() { render_device->end_frame(); }

    StaticMeshRenderableComponent Renderer::create_static_mesh(const std::vector<BveVertex>& vertices,
                                                               const std::vector<uint32_t>& indices) const {
        MTR_SCOPE("Renderer", "create_static_mesh");
        const auto mesh_start_idx = static_mesh_storage->add_mesh(vertices, indices);

        StaticMeshRenderableComponent renderable{};
        renderable.first_index = mesh_start_idx;
        renderable.num_indices = static_cast<uint32_t>(indices.size());

        return renderable;
    }

    TextureHandle Renderer::create_image(const rhi::ImageCreateInfo& create_info) {
        const auto idx = static_cast<uint32_t>(all_images.size());

        all_images.push_back(render_device->create_image(create_info));
        image_name_to_index.emplace(create_info.name, idx);

        return {idx};
    }

    rhi::Image& Renderer::get_image(const std::string& image_name) const {
        if(!image_name_to_index.contains(image_name)) {
            const auto message = fmt::format("Image '{}' does not exist", image_name);
            throw std::exception(message.c_str());
        }

        const auto idx = image_name_to_index.at(image_name);
        return *all_images[idx];
    }

    void Renderer::create_static_mesh_storage() {
        rhi::BufferCreateInfo vertex_create_info{};
        vertex_create_info.name = "Static Mesh Vertex Buffer";
        vertex_create_info.size = STATIC_MESH_VERTEX_BUFFER_SIZE;
        vertex_create_info.usage = rhi::BufferUsage::VertexBuffer;

        auto vertex_buffer = render_device->create_buffer(vertex_create_info);

        rhi::BufferCreateInfo index_buffer_create_info{};
        index_buffer_create_info.name = "Static Mesh Index Buffer";
        index_buffer_create_info.size = STATIC_MESH_INDEX_BUFFER_SIZE;
        index_buffer_create_info.usage = rhi::BufferUsage::IndexBuffer;

        auto index_buffer = render_device->create_buffer(index_buffer_create_info);

        static_mesh_storage = std::make_unique<rhi::MeshDataStore>(*render_device, std::move(vertex_buffer), std::move(index_buffer));
    }

    void Renderer::create_material_data_buffers() {
        material_data_buffer = std::make_unique<MaterialDataBuffer>(MATERIAL_DATA_BUFFER_SIZE);

        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer, .size = MATERIAL_DATA_BUFFER_SIZE};
        material_device_buffers.reserve(settings.num_in_flight_frames);
        for(uint32_t i = 0; i < settings.num_in_flight_frames; i++) {
            create_info.name = fmt::format("Material Data Buffer {}", 1);
            material_device_buffers.push_back(render_device->create_buffer(create_info));
        }
    }

    void Renderer::create_standard_pipeline() {
        const auto standard_pipeline_create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Standard material pipeline",
            .vertex_shader = load_shader("data/shaders/standard.vertex"),
            .pixel_shader = load_shader("data/shaders/standard.pixel"),
            .render_target_formats = {rhi::ImageFormat::Rgba8},
            .depth_stencil_format = rhi::ImageFormat::Depth32,
        };

        standard_pipeline = render_device->create_render_pipeline_state(standard_pipeline_create_info);

        spdlog::info("Created standard pipeline");
    }

    void Renderer::create_backbuffer_output_pipeline_and_material() {
        const auto create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Backbuffer output",
            .vertex_shader = load_shader("data/shaders/fullscreen.vertex"),
            .pixel_shader = load_shader("data/shaders/backbuffer_output.pixel"),
            .render_target_formats = {rhi::ImageFormat::Rgba8},
        };

        backbuffer_output_pipeline = render_device->create_render_pipeline_state(create_info);

        backbuffer_output_material.handle = material_data_buffer->get_next_free_index<BackbufferOutputMaterial>();
        material_data_buffer->at<BackbufferOutputMaterial>(backbuffer_output_material.handle)
            .scene_output_index = image_name_to_index[SCENE_COLOR_RENDER_TARGET];
    }

    void Renderer::create_scene_framebuffer(const glm::uvec2 size) {
        rhi::ImageCreateInfo color_target_create_info{};
        color_target_create_info.name = SCENE_COLOR_RENDER_TARGET;
        color_target_create_info.usage = rhi::ImageUsage::RenderTarget;
        color_target_create_info.format = rhi::ImageFormat::Rgba8;
        color_target_create_info.width = size.x;
        color_target_create_info.height = size.y;

        create_image(color_target_create_info);

        rhi::ImageCreateInfo depth_target_create_info{};
        depth_target_create_info.name = SCENE_DEPTH_TARGET;
        depth_target_create_info.usage = rhi::ImageUsage::DepthStencil;
        depth_target_create_info.format = rhi::ImageFormat::Depth32;
        depth_target_create_info.width = size.x;
        depth_target_create_info.height = size.y;

        scene_depth_target = render_device->create_image(depth_target_create_info);

        const auto& color_target = get_image(SCENE_COLOR_RENDER_TARGET);

        scene_framebuffer = render_device->create_framebuffer({&color_target}, scene_depth_target.get());
    }


    std::vector<const rhi::Image*> Renderer::get_texture_array() const {
        std::vector<const rhi::Image*> images;
        images.reserve(all_images.size());

        for(const auto& image : all_images) {
            images.push_back(image.get());
        }

        return images;
    }

    void Renderer::update_cameras(entt::registry& registry, rhi::RenderCommandList& commands, const uint32_t frame_idx) const {
        MTR_SCOPE("Renderer", "update_cameras");
        const auto camera_view = registry.view<TransformComponent, CameraComponent>();
        for(const auto entity : camera_view) {
            const auto& transform = registry.get<TransformComponent>(entity);
            const auto& camera = registry.get<CameraComponent>(entity);

            auto& matrices = camera_matrix_buffers->get_camera_matrices(camera.idx);
            matrices.previous_projection_matrix = matrices.projection_matrix;
            matrices.previous_view_matrix = matrices.view_matrix;
            matrices.calculate_view_matrix(transform);
            matrices.calculate_projection_matrix(camera);
        }

        camera_matrix_buffers->record_data_upload(commands, frame_idx);
    }

    void Renderer::render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list, const uint32_t frame_idx) const {
        MTR_SCOPE("Renderer", "render_3d_scene");

        static auto render_target_accesses = std::vector<rhi::RenderTargetAccess>{
            {.begin = {.type = rhi::RenderTargetBeginningAccessType::Clear, .clear_color = {0, 0, 0, 0}, .format = rhi::ImageFormat::Rgba8},
             .end = {.type = rhi::RenderTargetEndingAccessType::Preserve, .resolve_params = {}}}};

        static auto depth_access = rhi::RenderTargetAccess{.begin = {.type = rhi::RenderTargetBeginningAccessType::Clear,
                                                                     .clear_color = {1, 0, 0, 0},
                                                                     .format = rhi::ImageFormat::Depth32},
                                                           .end = {.type = rhi::RenderTargetEndingAccessType::Discard,
                                                                   .resolve_params = {}}};

        auto& material_bind_group_builder = render_device->get_material_bind_group_builder_for_frame(frame_idx);
        material_bind_group_builder.set_buffer("cameras", camera_matrix_buffers->get_device_buffer_for_frame(frame_idx));
        material_bind_group_builder.set_buffer("material_buffer", *material_device_buffers[frame_idx]);
        material_bind_group_builder.set_image_array("textures", get_texture_array());

        const auto material_bind_group = material_bind_group_builder.build();

        command_list.set_framebuffer(*scene_framebuffer, render_target_accesses, depth_access);

        command_list.set_pipeline_state(*standard_pipeline);
        command_list.set_camera_idx(0);
        command_list.bind_render_resources(*material_bind_group);

        command_list.bind_mesh_data(*static_mesh_storage);

        auto static_mesh_view = registry.view<StaticMeshRenderableComponent>();

        for(const auto entity : static_mesh_view) {
            const auto mesh_renderable = registry.get<StaticMeshRenderableComponent>(entity);

            command_list.draw(mesh_renderable.num_indices, mesh_renderable.first_index);
        }

        const auto* framebuffer = render_device->get_backbuffer_framebuffer();
        command_list.set_framebuffer(*framebuffer, {render_target_accesses});
        command_list.set_pipeline_state(*backbuffer_output_pipeline);

        command_list.draw(3);
    }
} // namespace renderer

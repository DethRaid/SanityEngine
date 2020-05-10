#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <entt/entity/registry.hpp>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "../core/components.hpp"
#include "../core/constants.hpp"
#include "../core/ensure.hpp"
#include "../rhi/d3dx12.hpp"
#include "../rhi/render_command_list.hpp"
#include "../rhi/render_device.hpp"
#include "camera_matrix_buffer.hpp"
#include "render_components.hpp"
#include "../loading/shader_loading.hpp"

namespace renderer {
    constexpr const char* SCENE_COLOR_RENDER_TARGET = "Scene color target";
    constexpr const char* SCENE_DEPTH_TARGET = "Scene depth target";

    constexpr uint32_t MATERIAL_DATA_BUFFER_SIZE = 1 << 20;

    struct BackbufferOutputMaterial {
        ImageHandle scene_output_image;
    };

#pragma region Cube
    std::vector<BveVertex> Renderer::cube_vertices{
        // Front
        {.position = {-0.5f, 0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, -0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, -0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, 0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Right
        {.position = {-0.5f, -0.5f, -0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, 0.5f, 0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, -0.5f, 0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, 0.5f, -0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Left
        {.position = {0.5f, 0.5f, 0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, -0.5f, -0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, -0.5f, 0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, 0.5f, -0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Back
        {.position = {0.5f, 0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, -0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, -0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, 0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Top
        {.position = {-0.5f, 0.5f, -0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, 0.5f, 0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, 0.5f, -0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, 0.5f, 0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Bottom
        {.position = {0.5f, -0.5f, 0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, -0.5f, -0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {0.5f, -0.5f, -0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        {.position = {-0.5f, -0.5f, 0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
    };

    std::vector<uint32_t> Renderer::cube_indices{
        // front face
        0,
        1,
        2, // first triangle
        0,
        3,
        1, // second triangle

        // left face
        4,
        5,
        6, // first triangle
        4,
        7,
        5, // second triangle

        // right face
        8,
        9,
        10, // first triangle
        8,
        11,
        9, // second triangle

        // back face
        12,
        13,
        14, // first triangle
        12,
        15,
        13, // second triangle

        // top face
        16,
        18,
        17, // first triangle
        16,
        17,
        19, // second triangle

        // bottom face
        20,
        21,
        22, // first triangle
        20,
        23,
        21, // second triangle
    };
#pragma endregion

    Renderer::Renderer(GLFWwindow* window, const Settings& settings_in)
        : logger{spdlog::stdout_color_st("Renderer")},
          settings{settings_in},
          device{make_render_device(rhi::RenderBackend::D3D12, window, settings_in)},
          camera_matrix_buffers{std::make_unique<CameraMatrixBuffer>(*device, settings_in.num_in_flight_gpu_frames)} {
        MTR_SCOPE("Renderer", "Renderer");

        create_static_mesh_storage();

        create_material_data_buffers();

        create_standard_pipeline();

        create_atmospheric_sky_pipeline();

        ENSURE(settings_in.render_scale > 0, "Render scale may not be 0 or less");

        int framebuffer_width;
        int framebuffer_height;
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        create_scene_framebuffer(glm::uvec2{framebuffer_width * settings_in.render_scale, framebuffer_height * settings_in.render_scale});

        create_backbuffer_output_pipeline_and_material();

        create_light_buffers();
    }

    void Renderer::begin_frame(const uint64_t frame_count) { device->begin_frame(frame_count); }

    void Renderer::render_scene(entt::registry& registry) {
        MTR_SCOPE("Renderer", "render_scene");

        const auto frame_idx = device->get_cur_gpu_frame_idx();

        auto command_list = device->create_render_command_list();
        command_list->set_debug_name("Main Render Command List");

        if(raytracing_scene_dirty) {
            rebuild_raytracing_scene(*command_list);
            raytracing_scene_dirty = false;
        }

        update_cameras(registry, *command_list, frame_idx);

        upload_material_data(*command_list, frame_idx);

        render_3d_scene(registry, *command_list, frame_idx);

        device->submit_command_list(std::move(command_list));
    }

    void Renderer::end_frame() { device->end_frame(); }

    void Renderer::add_raytracing_objects_to_scene(const std::vector<rhi::RaytracingObject>& new_objects) {
        raytracing_objects.insert(raytracing_objects.end(), new_objects.begin(), new_objects.end());
        raytracing_scene_dirty = true;
    }

    rhi::Mesh Renderer::create_static_mesh(const std::vector<BveVertex>& vertices,
                                           const std::vector<uint32_t>& indices,
                                           rhi::ResourceCommandList& commands) const {
        MTR_SCOPE("Renderer", "create_static_mesh");

        const auto& [vertex_offset, index_offset] = static_mesh_storage->add_mesh(vertices, indices, commands);

        return {.first_vertex = vertex_offset,
                .num_vertices = static_cast<uint32_t>(vertices.size()),
                .first_index = index_offset,
                .num_indices = static_cast<uint32_t>(indices.size())};
    }

    ImageHandle Renderer::create_image(const rhi::ImageCreateInfo& create_info) {
        const auto idx = static_cast<uint32_t>(all_images.size());

        all_images.push_back(device->create_image(create_info));
        image_name_to_index.emplace(create_info.name, idx);

        return {idx};
    }

    ImageHandle Renderer::create_image(const rhi::ImageCreateInfo& create_info, const void* image_data) {
        const auto handle = create_image(create_info);
        auto& image = *all_images[handle.idx];

        auto commands = device->create_resource_command_list();
        commands->copy_data_to_image(image_data, image);
        device->submit_command_list(std::move(commands));

        return handle;
    }

    std::optional<ImageHandle> Renderer::get_image_handle(const std::string& name) {
        if(const auto itr = image_name_to_index.find(name); itr != image_name_to_index.end()) {
            return ImageHandle{itr->second};

        } else {
            return std::nullopt;
        }
    }

    rhi::Image& Renderer::get_image(const std::string& image_name) const {
        if(!image_name_to_index.contains(image_name)) {
            const auto message = fmt::format("Image '{}' does not exist", image_name);
            throw std::exception(message.c_str());
        }

        const auto idx = image_name_to_index.at(image_name);
        return *all_images[idx];
    }

    rhi::Image& Renderer::get_image(const ImageHandle handle) const { return *all_images[handle.idx]; }

    MaterialDataBuffer& Renderer::get_material_data_buffer() const { return *material_data_buffer; }

    rhi::RenderDevice& Renderer::get_render_device() const { return *device; }

    rhi::MeshDataStore& Renderer::get_static_mesh_store() const { return *static_mesh_storage; }

    void Renderer::begin_device_capture() const { device->begin_capture(); }

    void Renderer::end_device_capture() const { device->end_capture(); }

    void Renderer::create_static_mesh_storage() {
        const auto vertex_create_info = rhi::BufferCreateInfo{
            .name = "Static Mesh Vertex Buffer",
            .usage = rhi::BufferUsage::VertexBuffer,
            .size = STATIC_MESH_VERTEX_BUFFER_SIZE,
        };

        auto vertex_buffer = device->create_buffer(vertex_create_info);

        const auto index_buffer_create_info = rhi::BufferCreateInfo{.name = "Static Mesh Index Buffer",
                                                                    .usage = rhi::BufferUsage::IndexBuffer,
                                                                    .size = STATIC_MESH_INDEX_BUFFER_SIZE};

        auto index_buffer = device->create_buffer(index_buffer_create_info);

        static_mesh_storage = std::make_unique<rhi::MeshDataStore>(*device, std::move(vertex_buffer), std::move(index_buffer));
    }

    void Renderer::create_material_data_buffers() {
        material_data_buffer = std::make_unique<MaterialDataBuffer>(MATERIAL_DATA_BUFFER_SIZE);

        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer, .size = MATERIAL_DATA_BUFFER_SIZE};
        material_device_buffers.reserve(settings.num_in_flight_gpu_frames);
        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            create_info.name = fmt::format("Material Data Buffer {}", 1);
            material_device_buffers.push_back(device->create_buffer(create_info));
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

        standard_pipeline = device->create_render_pipeline_state(standard_pipeline_create_info);

        logger->info("Created standard pipeline");
    }

    void Renderer::create_atmospheric_sky_pipeline() {
        const auto atmospheric_sky_create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Atmospheric Sky",
            .vertex_shader = load_shader("data/shaders/fullscreen.vertex"),
            .pixel_shader = load_shader("data/shaders/atmospheric_sky.pixel"),
            .depth_stencil_state = {.enable_depth_test = true, .enable_depth_write = false, .depth_func = rhi::CompareOp::LessOrEqual},
            .render_target_formats = {rhi::ImageFormat::Rgba8},
            .depth_stencil_format = rhi::ImageFormat::Depth32,
        };

        atmospheric_sky_pipeline = device->create_render_pipeline_state(atmospheric_sky_create_info);
    }

    void Renderer::create_backbuffer_output_pipeline_and_material() {
        const auto create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Backbuffer output",
            .vertex_shader = load_shader("data/shaders/fullscreen.vertex"),
            .pixel_shader = load_shader("data/shaders/backbuffer_output.pixel"),
            .render_target_formats = {rhi::ImageFormat::Rgba8},
        };

        backbuffer_output_pipeline = device->create_render_pipeline_state(create_info);

        backbuffer_output_material = material_data_buffer->get_next_free_material<BackbufferOutputMaterial>();
        material_data_buffer->at<BackbufferOutputMaterial>(backbuffer_output_material).scene_output_image = {
            image_name_to_index[SCENE_COLOR_RENDER_TARGET]};
    }

    void Renderer::create_light_buffers() {
        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer, .size = MAX_NUM_LIGHTS * sizeof(Light)};

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            create_info.name = fmt::format("Light Buffer {}", i);
            light_device_buffers.push_back(device->create_buffer(create_info));
        }
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

        scene_depth_target = device->create_image(depth_target_create_info);

        const auto& color_target = get_image(SCENE_COLOR_RENDER_TARGET);

        scene_framebuffer = device->create_framebuffer({&color_target}, scene_depth_target.get());
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

            matrices.copy_matrices_to_previous();
            matrices.calculate_view_matrix(transform);
            matrices.calculate_projection_matrix(camera);
        }

        camera_matrix_buffers->record_data_upload(commands, frame_idx);
    }

    void Renderer::upload_material_data(rhi::ResourceCommandList& command_list, const uint32_t frame_idx) {
        auto& buffer = *material_device_buffers[frame_idx];

        command_list.copy_data_to_buffer(material_data_buffer->data(), material_data_buffer->size(), buffer, 0);
    }

    void Renderer::rebuild_raytracing_scene(rhi::RenderCommandList& command_list) {
        // TODO: figure out how to update the raytracing scene without needing a full rebuild

        if(raytracing_scene.buffer) {
            device->destroy_buffer(std::move(raytracing_scene.buffer));
        }

        raytracing_scene = command_list.build_raytracing_scene(raytracing_objects);
    }

    void Renderer::update_lights(entt::registry& registry, const uint32_t frame_idx) {
        registry.view<LightComponent>().each([&](const LightComponent& light) { lights[light.handle.handle] = light.light; });

        auto* dst = device->map_buffer(*light_device_buffers[frame_idx]);
        std::memcpy(dst, lights.data(), lights.size() * sizeof(Light));
    }

    void Renderer::draw_sky(entt::registry& registry, rhi::RenderCommandList& command_list) {
        const auto atmosphere_view = registry.view<AtmosphericSkyComponent>();
        if(atmosphere_view.size() > 1) {
            logger->error("May only have one atmospheric sky component in a scene");
        } else {
            command_list.set_pipeline_state(*atmospheric_sky_pipeline);
            command_list.draw(3);
        }
    }

    void Renderer::render_3d_scene(entt::registry& registry, rhi::RenderCommandList& command_list, const uint32_t frame_idx) {
        MTR_SCOPE("Renderer", "render_3d_scene");

        update_lights(registry, frame_idx);

        static auto render_target_accesses = std::vector<rhi::RenderTargetAccess>{
            {.begin = {.type = rhi::RenderTargetBeginningAccessType::Clear, .clear_color = {0, 0, 0, 0}, .format = rhi::ImageFormat::Rgba8},
             .end = {.type = rhi::RenderTargetEndingAccessType::Preserve, .resolve_params = {}}}};

        static auto depth_access = rhi::RenderTargetAccess{.begin = {.type = rhi::RenderTargetBeginningAccessType::Clear,
                                                                     .clear_color = {1, 0, 0, 0},
                                                                     .format = rhi::ImageFormat::Depth32},
                                                           .end = {.type = rhi::RenderTargetEndingAccessType::Discard,
                                                                   .resolve_params = {}}};

        auto& material_bind_group_builder = device->get_material_bind_group_builder_for_frame(frame_idx);
        material_bind_group_builder.set_buffer("cameras", camera_matrix_buffers->get_device_buffer_for_frame(frame_idx));
        material_bind_group_builder.set_buffer("material_buffer", *material_device_buffers[frame_idx]);
        material_bind_group_builder.set_buffer("lights", *light_device_buffers[frame_idx]);
        material_bind_group_builder.set_raytracing_scene("raytracing_scene", raytracing_scene);
        material_bind_group_builder.set_image_array("textures", get_texture_array());

        const auto material_bind_group = material_bind_group_builder.build();

        command_list.set_framebuffer(*scene_framebuffer, render_target_accesses, depth_access);

        command_list.set_pipeline_state(*standard_pipeline);
        command_list.set_camera_idx(0);
        command_list.bind_render_resources(*material_bind_group);

        command_list.bind_mesh_data(*static_mesh_storage);

        registry.view<StaticMeshRenderableComponent>().each([&](const StaticMeshRenderableComponent& mesh_renderable) {
            command_list.set_material_idx(mesh_renderable.material.index);
            command_list.draw(mesh_renderable.mesh.num_indices, mesh_renderable.mesh.first_index);
        });

        draw_sky(registry, command_list);

        const auto* framebuffer = device->get_backbuffer_framebuffer();
        command_list.set_framebuffer(*framebuffer, {render_target_accesses});
        command_list.set_material_idx(backbuffer_output_material.index);
        command_list.set_pipeline_state(*backbuffer_output_pipeline);

        command_list.draw(3);
    }
} // namespace renderer

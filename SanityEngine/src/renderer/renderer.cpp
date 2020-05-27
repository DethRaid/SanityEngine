#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <cuda.h>
#include <entt/entity/registry.hpp>
#include <imgui/imgui.h>
#include <minitrace.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "../core/align.hpp"
#include "../core/components.hpp"
#include "../core/constants.hpp"
#include "../core/defer.hpp"
#include "../core/ensure.hpp"
#include "../core/errors.hpp"
#include "../loading/image_loading.hpp"
#include "../loading/shader_loading.hpp"
#include "../rhi/d3dx12.hpp"
#include "../rhi/helpers.hpp"
#include "../rhi/render_command_list.hpp"
#include "../rhi/render_device.hpp"
#include "camera_matrix_buffer.hpp"
#include "render_components.hpp"

static_assert(sizeof(CUdeviceptr) == sizeof(void*));

namespace renderer {
    constexpr const char* SCENE_COLOR_RENDER_TARGET = "Scene color target";
    constexpr const char* SCENE_DEPTH_TARGET = "Scene depth target";

    constexpr const char* ACCUMULATION_RENDER_TARGET = "Accumulation target";
    constexpr const char* DENOISED_SCENE_RENDER_TARGET = "Denoised scene color target";

    constexpr uint32_t MATERIAL_DATA_BUFFER_SIZE = 1 << 20;

    struct BackbufferOutputMaterial {
        TextureHandle scene_output_image;
    };

    struct AccumulationMaterial {
        TextureHandle accumulation_texture;
        TextureHandle scene_output_texture;
        TextureHandle scene_depth_texture;
    };

#pragma region Cube
    std::vector<StandardVertex> Renderer::cube_vertices{
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
          start_time{std::chrono::high_resolution_clock::now()},
          settings{settings_in},
          device{make_render_device(rhi::RenderBackend::D3D12, window, settings_in)},
          camera_matrix_buffers{std::make_unique<CameraMatrixBuffer>(*device, settings_in.num_in_flight_gpu_frames)} {
        MTR_SCOPE("Renderer", "Renderer");

        // logger->set_level(spdlog::level::debug);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        output_framebuffer_size = {static_cast<uint32_t>(width * settings.render_scale),
                                   static_cast<uint32_t>(height * settings.render_scale)};

        if(settings.use_optix_denoiser) {
            create_optix_context();
        }

        create_static_mesh_storage();

        create_per_frame_data_buffers();

        create_material_data_buffers();

        create_standard_pipeline();

        create_atmospheric_sky_pipeline();

        create_ui_pipeline();

        create_ui_mesh_buffers();

        ENSURE(settings_in.render_scale > 0, "Render scale may not be 0 or less");

        create_scene_framebuffer();

        create_accumulation_pipeline_and_material();

        create_shadowmap_framebuffer_and_pipeline(settings.shadow_quality);

        create_backbuffer_output_pipeline_and_material();

        create_light_buffers();

        create_builtin_images();
    }

    void Renderer::load_noise_texture(const std::string& filepath) {
        uint32_t width, height;
        std::vector<uint8_t> pixels;

        const auto success = load_image(filepath, width, height, pixels);
        if(!success) {
            logger->error("Could not load noise texture {}", filepath);
            return;
        }

        auto commands = device->create_command_list();

        noise_texture_handle = create_image(rhi::ImageCreateInfo{.name = "Noise Texture",
                                                                 .usage = rhi::ImageUsage::SampledImage,
                                                                 .format = rhi::ImageFormat::Rgba8,
                                                                 .width = width,
                                                                 .height = height},
                                            pixels.data(),
                                            commands);

        device->submit_command_list(std::move(commands));
    }

    void Renderer::begin_frame(const uint64_t frame_count) {
        device->begin_frame(frame_count);

        const auto cur_time = std::chrono::high_resolution_clock::now();
        const auto duration_since_start = cur_time - start_time;
        const auto ns_since_start = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_start).count();
        const auto time_since_start = static_cast<double>(ns_since_start) / 1000000000.0;

        per_frame_data.time_since_start = static_cast<float>(time_since_start);
    }

    void Renderer::render_all(entt::registry& registry) {
        MTR_SCOPE("Renderer", "render_scene");

        const auto frame_idx = device->get_cur_gpu_frame_idx();

        auto command_list = device->create_command_list();
        command_list->SetName(L"Main Render Command List");

        if(raytracing_scene_dirty) {
            rebuild_raytracing_scene(command_list);
            raytracing_scene_dirty = false;
        }

        update_cameras(registry, frame_idx);

        upload_material_data(frame_idx);

        // render_3d_scene binds all the render targets it needs
        render_3d_scene(registry, command_list, frame_idx);

        // At the end of render_3d_scene, the backbuffer framebuffer is bound. render_ui thus simply renders directly onto the backbuffer
        render_ui(command_list, frame_idx);

        device->submit_command_list(std::move(command_list));
    }

    void Renderer::end_frame() const { device->end_frame(); }

    void Renderer::add_raytracing_objects_to_scene(const std::vector<rhi::RaytracingObject>& new_objects) {
        raytracing_objects.insert(raytracing_objects.end(), new_objects.begin(), new_objects.end());
        raytracing_scene_dirty = true;
    }

    TextureHandle Renderer::create_image(const rhi::ImageCreateInfo& create_info) {
        const auto idx = static_cast<uint32_t>(all_images.size());

        all_images.push_back(device->create_image(create_info));
        image_name_to_index.emplace(create_info.name, idx);

        logger->debug("Created texture {} with index {}", create_info.name, idx);

        return {idx};
    }

    TextureHandle Renderer::create_image(const rhi::ImageCreateInfo& create_info,
                                         const void* image_data,
                                         const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        const auto handle = create_image(create_info);
        auto& image = *all_images[handle.index];

        const auto num_bytes_in_texture = create_info.width * create_info.height * rhi::size_in_bytes(create_info.format);

        const auto& staging_buffer = device->get_staging_buffer(num_bytes_in_texture);

        const auto subresource = D3D12_SUBRESOURCE_DATA{
            .pData = image_data,
            .RowPitch = create_info.width * 4,
            .SlicePitch = create_info.width * create_info.height * 4,
        };

        const auto result = UpdateSubresources(commands.Get(), image.resource.Get(), staging_buffer.resource.Get(), 0, 0, 1, &subresource);
        if(result == 0 || FAILED(result)) {
            spdlog::error("Could not upload texture data");
        }

        return handle;
    }

    std::optional<TextureHandle> Renderer::get_image_handle(const std::string& name) {
        if(const auto itr = image_name_to_index.find(name); itr != image_name_to_index.end()) {
            return TextureHandle{itr->second};

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

    rhi::Image& Renderer::get_image(const TextureHandle handle) const { return *all_images[handle.index]; }

    void Renderer::schedule_texture_destruction(const TextureHandle& image_handle) {
        auto image = std::move(all_images[image_handle.index]);
        device->schedule_image_destruction(std::move(image));
    }

    MaterialDataBuffer& Renderer::get_material_data_buffer() const { return *material_data_buffer; }

    rhi::RenderDevice& Renderer::get_render_device() const { return *device; }

    rhi::MeshDataStore& Renderer::get_static_mesh_store() const { return *static_mesh_storage; }

    void Renderer::begin_device_capture() const { device->begin_capture(); }

    void Renderer::end_device_capture() const { device->end_capture(); }

    TextureHandle Renderer::get_noise_texture() const { return noise_texture_handle; }

    TextureHandle Renderer::get_pink_texture() const { return pink_texture_handle; }

    TextureHandle Renderer::get_default_normal_roughness_texture() const { return normal_roughness_texture_handle; }

    TextureHandle Renderer::get_default_specular_color_emission_texture() const { return specular_emission_texture_handle; }

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

    void Renderer::create_per_frame_data_buffers() {
        per_frame_data_buffers.reserve(settings.num_in_flight_gpu_frames);

        auto create_info = rhi::BufferCreateInfo{
            .usage = rhi::BufferUsage::ConstantBuffer,
            .size = sizeof(PerFrameData),
        };

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            create_info.name = fmt::format("Per frame data buffer {}", i);
            per_frame_data_buffers.push_back(device->create_buffer(create_info));
        }
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
            .vertex_shader = load_shader("standard.vertex"),
            .pixel_shader = load_shader("standard.pixel"),
            // .blend_state = {.render_target_blends = {rhi::RenderTargetBlendState{.enabled = true}}},
            .render_target_formats = {rhi::ImageFormat::Rgba32F},
            .depth_stencil_format = rhi::ImageFormat::Depth32,
        };

        standard_pipeline = device->create_render_pipeline_state(standard_pipeline_create_info);

        logger->info("Created standard pipeline");
    }

    void Renderer::create_atmospheric_sky_pipeline() {
        const auto atmospheric_sky_create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Atmospheric Sky",
            .vertex_shader = load_shader("fullscreen.vertex"),
            .pixel_shader = load_shader("atmospheric_sky.pixel"),
            .depth_stencil_state = {.enable_depth_test = true, .enable_depth_write = false, .depth_func = rhi::CompareOp::LessOrEqual},
            .render_target_formats = {rhi::ImageFormat::Rgba32F},
            .depth_stencil_format = rhi::ImageFormat::Depth32,
        };

        atmospheric_sky_pipeline = device->create_render_pipeline_state(atmospheric_sky_create_info);
    }

    void Renderer::create_backbuffer_output_pipeline_and_material() {
        const auto create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Backbuffer output",
            .vertex_shader = load_shader("fullscreen.vertex"),
            .pixel_shader = load_shader("backbuffer_output.pixel"),
            .render_target_formats = {rhi::ImageFormat::Rgba8},
        };

        backbuffer_output_pipeline = device->create_render_pipeline_state(create_info);

        backbuffer_output_material = material_data_buffer->get_next_free_material<BackbufferOutputMaterial>();
        material_data_buffer->at<BackbufferOutputMaterial>(backbuffer_output_material).scene_output_image = denoised_color_target_handle;

        logger->debug("Initialized backbuffer output pass");
    }

    void Renderer::create_light_buffers() {
        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer, .size = MAX_NUM_LIGHTS * sizeof(Light)};

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            create_info.name = fmt::format("Light Buffer {}", i);
            light_device_buffers.push_back(device->create_buffer(create_info));
        }
    }

    void Renderer::create_builtin_images() {
        load_noise_texture("data/textures/LDR_RGBA_0.png");

        const auto commands = device->create_command_list();

        {
            const auto pink_texture_create_info = rhi::ImageCreateInfo{.name = "Pink",
                                                                       .usage = rhi::ImageUsage::SampledImage,
                                                                       .format = rhi::ImageFormat::Rgba8,
                                                                       .width = 8,
                                                                       .height = 8};

            const auto pink_texture_pixel = std::vector<uint32_t>(static_cast<size_t>(64), 0xFFFF00FF);

            pink_texture_handle = create_image(pink_texture_create_info, pink_texture_pixel.data(), commands);
        }

        {
            const auto normal_roughness_texture_create_info = rhi::ImageCreateInfo{.name = "Default Normal/Roughness",
                                                                                   .usage = rhi::ImageUsage::SampledImage,
                                                                                   .format = rhi::ImageFormat::Rgba8,
                                                                                   .width = 8,
                                                                                   .height = 8};

            const auto normal_roughness_texture_pixel = std::vector<uint32_t>(static_cast<size_t>(64), 0x80FF8080);

            normal_roughness_texture_handle = create_image(normal_roughness_texture_create_info,
                                                           normal_roughness_texture_pixel.data(),
                                                           commands);
        }

        {
            const auto specular_emission_texture_create_info = rhi::ImageCreateInfo{.name = "Default Specular Color/Emission",
                                                                                    .usage = rhi::ImageUsage::SampledImage,
                                                                                    .format = rhi::ImageFormat::Rgba8,
                                                                                    .width = 8,
                                                                                    .height = 8};

            const auto specular_emission_texture_pixel = std::vector<uint32_t>(static_cast<size_t>(64), 0x00373737);

            specular_emission_texture_handle = create_image(specular_emission_texture_create_info,
                                                            specular_emission_texture_pixel.data(),
                                                            commands);
        }

        device->submit_command_list(commands);
    }

    bool Renderer::expose_render_target_to_optix(const rhi::Image& render_target, void** mapped_pointer) const {
        HANDLE shared_handle;
        {
            const auto result = device->device5->CreateSharedHandle(render_target.resource.Get(),
                                                                    nullptr,
                                                                    GENERIC_ALL,
                                                                    nullptr,
                                                                    &shared_handle);
            if(FAILED(result)) {
                logger->error("Could not create shared handle to image {}: {}", render_target.name, to_string(result));
                return false;
            }
        }

        const auto total_size = render_target.width * render_target.height * 4 * 4;
        const auto cuda_color_target_handle_desc = cudaExternalMemoryHandleDesc{.type = cudaExternalMemoryHandleTypeD3D12Resource,
                                                                                .handle = {.win32 = {.handle = shared_handle}},
                                                                                .size = total_size,
                                                                                .flags = cudaExternalMemoryDedicated};

        cudaExternalMemory_t render_target_memory;
        auto result = cudaImportExternalMemory(&render_target_memory, &cuda_color_target_handle_desc);
        if(result != cudaSuccess) {
            const auto* const error_name = cudaGetErrorName(result);
            const auto* const error = cudaGetErrorString(result);
            logger->error("Could not share scene render target: {}: {}", error_name, error);
            return false;

        } else {
            CloseHandle(shared_handle);
        }

        const auto buffer_desc = cudaExternalMemoryBufferDesc{.offset = 0, .size = total_size};
        result = cudaExternalMemoryGetMappedBuffer(mapped_pointer, render_target_memory, &buffer_desc);
        if(result != cudaSuccess) {
            const auto* const error_name = cudaGetErrorName(result);
            const auto* const error = cudaGetErrorString(result);
            logger->error("Could not map scene render target to a device pointer: {}: {}", error_name, error);
            return false;
        }

        return true;
    }

    void Renderer::create_scene_framebuffer() {
        auto color_target_create_info = rhi::ImageCreateInfo{
            .name = SCENE_COLOR_RENDER_TARGET,
            .usage = rhi::ImageUsage::RenderTarget,
            .format = rhi::ImageFormat::Rgba32F,
            .width = output_framebuffer_size.x,
            .height = output_framebuffer_size.y,
            .enable_resource_sharing = true,
        };

        scene_color_target_handle = create_image(color_target_create_info);

        const auto depth_target_create_info = rhi::ImageCreateInfo{
            .name = SCENE_DEPTH_TARGET,
            .usage = rhi::ImageUsage::DepthStencil,
            .format = rhi::ImageFormat::Depth32,
            .width = output_framebuffer_size.x,
            .height = output_framebuffer_size.y,
        };

        scene_depth_target_handle = create_image(depth_target_create_info);

        const auto& color_target = get_image(scene_color_target_handle);
        const auto& depth_target = get_image(scene_depth_target_handle);

        scene_framebuffer = device->create_framebuffer({&color_target}, &depth_target);

        color_target_create_info.name = DENOISED_SCENE_RENDER_TARGET;
        denoised_color_target_handle = create_image(color_target_create_info);

        const auto& denoised_color_target = get_image(denoised_color_target_handle);
        denoised_framebuffer = device->create_framebuffer({&denoised_color_target});

        if(settings.use_optix_denoiser) {
            // Expose the scene color output to OptiX
            auto exposed = expose_render_target_to_optix(color_target, &mapped_scene_render_target);
            if(!exposed) {
                logger->error("Could not expose scene color target to OptiX");
            }

            // Expose the denoised scene output to OptiX
            exposed = expose_render_target_to_optix(denoised_color_target, &mapped_denoised_render_target);
            if(!exposed) {
                logger->error("Could not expose denoised color target to OptiX");
            }
        }
    }

    void Renderer::create_accumulation_pipeline_and_material() {
        const auto pipeline_create_info = rhi::RenderPipelineStateCreateInfo{.name = "Accumulation Pipeline",
                                                                             .vertex_shader = load_shader("fullscreen.vertex"),
                                                                             .pixel_shader = load_shader("raytracing_accumulation.pixel"),
                                                                             .depth_stencil_state = {.enable_depth_test = false,
                                                                                                     .enable_depth_write = false},
                                                                             .render_target_formats = {rhi::ImageFormat::Rgba32F}};

        accumulation_pipeline = device->create_render_pipeline_state(pipeline_create_info);

        const auto accumulation_target_create_info = rhi::ImageCreateInfo{
            .name = ACCUMULATION_RENDER_TARGET,
            .usage = rhi::ImageUsage::SampledImage,
            .format = rhi::ImageFormat::Rgba32F,
            .width = output_framebuffer_size.x,
            .height = output_framebuffer_size.y,
            .enable_resource_sharing = true,
        };
        accumulation_target_handle = create_image(accumulation_target_create_info);

        accumulation_material_handle = material_data_buffer->get_next_free_material<AccumulationMaterial>();
        auto& accumulation_material = material_data_buffer->at<AccumulationMaterial>(accumulation_material_handle);
        accumulation_material.accumulation_texture = accumulation_target_handle;
        accumulation_material.scene_output_texture = scene_color_target_handle;
        accumulation_material.scene_depth_texture = scene_depth_target_handle;

        logger->trace("Accumulation material idx: {} Accumulation texture idx: {} scene output texture idx: {}",
                      accumulation_material_handle.index,
                      accumulation_target_handle.index,
                      scene_color_target_handle.index);
    }

    void Renderer::create_shadowmap_framebuffer_and_pipeline(const QualityLevel quality_level) {
        {
            const auto shadowmap_resolution = [&]() {
                // TODO: Scale based on  current hardware
                switch(quality_level) {
                    case QualityLevel::Low:
                        return 512u;

                    case QualityLevel::Medium:
                        return 1024u;

                    case QualityLevel::High:
                        return 2048u;

                    case QualityLevel::Ultra:
                        return 4096u;
                }

                return 1024u;
            }();

            const auto create_info = rhi::ImageCreateInfo{
                .name = "Sun Shadowmap",
                .usage = rhi::ImageUsage::DepthStencil,
                .format = rhi::ImageFormat::Depth32,
                .width = shadowmap_resolution,
                .height = shadowmap_resolution,
            };

            shadow_map_image = device->create_image(create_info);

            shadow_map_framebuffer = device->create_framebuffer({}, shadow_map_image.get());
        }

        {
            const auto create_info = rhi::RenderPipelineStateCreateInfo{
                .name = "Shadow",
                .vertex_shader = load_shader("standard.vertex"),
                .pixel_shader = load_shader("shadow.pixel"),
                .depth_stencil_format = rhi::ImageFormat::Depth32,
            };

            shadow_pipeline = device->create_render_pipeline_state(create_info);
        }
    }

    std::vector<const rhi::Image*> Renderer::get_texture_array() const {
        std::vector<const rhi::Image*> images;
        images.reserve(all_images.size());

        for(const auto& image : all_images) {
            images.push_back(image.get());
        }

        return images;
    }

    void Renderer::update_cameras(entt::registry& registry, const uint32_t frame_idx) const {
        MTR_SCOPE("Renderer", "update_cameras");
        registry.view<TransformComponent, CameraComponent>().each([&](const TransformComponent& transform, const CameraComponent& camera) {
            auto& matrices = camera_matrix_buffers->get_camera_matrices(camera.idx);

            matrices.copy_matrices_to_previous();
            matrices.calculate_view_matrix(transform);
            matrices.calculate_projection_matrix(camera);
        });

        camera_matrix_buffers->upload_data(frame_idx);
    }

    void Renderer::upload_material_data(const uint32_t frame_idx) {
        auto& buffer = *material_device_buffers[frame_idx];
        memcpy(buffer.mapped_ptr, material_data_buffer->data(), material_data_buffer->size());
    }

    void Renderer::create_optix_context() {
        {

            const auto result = cudaFree(nullptr);
            if(result != cudaSuccess) {
                const auto* const error_name = cudaGetErrorName(result);
                const auto* const error = cudaGetErrorString(result);
                logger->error("Could not create CUDA context: {}: {}", error_name, error);
            }
        }

        {
            const auto result = optixInit();
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not create OptiX context: {}: {}", error_name, error);
                return;
            }
        }

        auto device_context_options = OptixDeviceContextOptions{
            .logCallbackFunction =
                +[](const unsigned int level, const char* tag, const char* message, void* data) {
                    constexpr uint32_t LOG_LEVEL_FATAL = 1;
                    constexpr uint32_t LOG_LEVEL_ERROR = 2;
                    constexpr uint32_t LOG_LEVEL_WARNING = 3;
                    constexpr uint32_t LOG_LEVEL_PRINT = 4;

                    auto* logger = static_cast<spdlog::logger*>(data);
                    if(level == LOG_LEVEL_FATAL || level == LOG_LEVEL_ERROR) {
                        logger->error("[{}]: {}", tag, message);

                    } else if(level == LOG_LEVEL_WARNING) {
                        logger->warn("[{}]: {}", tag, message);

                    } else {
                        logger->info("[{}]: {}", tag, message);
                    }
                },
            .logCallbackData = logger.get(),
#ifndef NDEBUG
            .logCallbackLevel = 4,
#else
            .logCallbackLevel = 3
#endif
        };

        {
            const auto result = optixDeviceContextCreate(nullptr, &device_context_options, &optix_context);
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not create OptiX context: {}: {}", error_name, error);
                return;
            }
        }

        {
            const auto denoiser_options = OptixDenoiserOptions{.inputKind = OPTIX_DENOISER_INPUT_RGB,
                                                               .pixelFormat = OPTIX_PIXEL_FORMAT_FLOAT3};
            const auto result = optixDenoiserCreate(optix_context, &denoiser_options, &optix_denoiser);
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not create OptiX denoiser: {}: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = optixDenoiserSetModel(optix_denoiser, OPTIX_DENOISER_MODEL_KIND_HDR, nullptr, 0);
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not set denoiser model: {}: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = optixDenoiserComputeMemoryResources(optix_denoiser,
                                                                    output_framebuffer_size.x,
                                                                    output_framebuffer_size.y,
                                                                    &optix_sizes);
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not compute denoiser memory sizes: {}: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = cudaStreamCreate(&denoiser_stream);
            if(result != cudaSuccess) {
                const auto* const error_name = cudaGetErrorName(result);
                const auto* const error = cudaGetErrorString(result);
                logger->error("Could not create CUDA stream - [{}]: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = cudaMalloc(reinterpret_cast<void**>(&denoiser_state), optix_sizes.stateSizeInBytes);
            if(result != cudaSuccess) {
                const auto* const error_name = cudaGetErrorName(result);
                const auto* const error = cudaGetErrorString(result);
                logger->error("Could not allocate OptiX state - [{}]: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = cudaMalloc(reinterpret_cast<void**>(&denoiser_scratch), optix_sizes.recommendedScratchSizeInBytes);
            if(result != cudaSuccess) {
                const auto* const error_name = cudaGetErrorName(result);
                const auto* const error = cudaGetErrorString(result);
                logger->error("Could not allocate OptiX scratch memory - [{}]: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = optixDenoiserSetup(optix_denoiser,
                                                   denoiser_stream,
                                                   output_framebuffer_size.x,
                                                   output_framebuffer_size.y,
                                                   denoiser_state,
                                                   optix_sizes.stateSizeInBytes,
                                                   denoiser_scratch,
                                                   optix_sizes.recommendedScratchSizeInBytes);
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not setup denoiser: {}: {}", error_name, error);
                return;
            }
        }

        {
            const auto result = cudaMalloc(reinterpret_cast<void**>(&intensity_ptr), sizeof(double));
            if(result != cudaSuccess) {
                const auto* const error_name = cudaGetErrorName(result);
                const auto* const error = cudaGetErrorString(result);
                logger->error("Could not allocate memory to hold intensity - [{}]: {}", error_name, error);
                return;
            }
        }
    }

    void Renderer::rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        // TODO: figure out how to update the raytracing scene without needing a full rebuild

        if(raytracing_scene.buffer) {
            device->schedule_buffer_destruction(std::move(raytracing_scene.buffer));
        }

        if(!raytracing_objects.empty()) {
            constexpr auto max_num_objects = UINT32_MAX / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

            ENSURE(raytracing_objects.size() < max_num_objects, "May not have more than {} objects because uint32", max_num_objects);

            const auto instance_buffer_size = static_cast<uint32_t>(raytracing_objects.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
            auto instance_buffer = device->get_staging_buffer(instance_buffer_size);
            auto* instance_buffer_array = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instance_buffer.ptr);

            for(uint32_t i = 0; i < raytracing_objects.size(); i++) {
                const auto& object = raytracing_objects[i];
                auto& desc = instance_buffer_array[i];
                desc = {};

                // TODO: Actually copy the matrix once we get real model matrices
                desc.Transform[0][0] = desc.Transform[1][1] = desc.Transform[2][2] = 1;

                // TODO: Figure out if we want to use the mask to control which kind of rays can hit which objects
                desc.InstanceMask = 0xFF;

                desc.InstanceContributionToHitGroupIndex = object.material.handle;

                const auto& buffer = static_cast<const rhi::Buffer&>(*object.blas_buffer);
                desc.AccelerationStructure = buffer.resource->GetGPUVirtualAddress();
            }

            const auto as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
                .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
                .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
                .NumDescs = static_cast<UINT>(raytracing_objects.size()),
                .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
                .InstanceDescs = instance_buffer.resource->GetGPUVirtualAddress(),
            };

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info{};
            device->device5->GetRaytracingAccelerationStructurePrebuildInfo(&as_inputs, &prebuild_info);

            prebuild_info.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                         prebuild_info.ScratchDataSizeInBytes);
            prebuild_info.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                           prebuild_info.ResultDataMaxSizeInBytes);

            auto scratch_buffer = device->get_scratch_buffer(static_cast<uint32_t>(prebuild_info.ScratchDataSizeInBytes));

            const auto as_buffer_create_info = rhi::BufferCreateInfo{.name = "Raytracing Scene",
                                                                     .usage = rhi::BufferUsage::RaytracingAccelerationStructure,
                                                                     .size = static_cast<uint32_t>(prebuild_info.ResultDataMaxSizeInBytes)};
            auto as_buffer = device->create_buffer(as_buffer_create_info);

            const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
                .DestAccelerationStructureData = as_buffer->resource->GetGPUVirtualAddress(),
                .Inputs = as_inputs,
                .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress(),
            };

            DEFER(a, [&]() { device->return_staging_buffer(std::move(instance_buffer)); });
            DEFER(b, [&]() { device->return_scratch_buffer(std::move(scratch_buffer)); });

            commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

            const auto barriers = std::vector<D3D12_RESOURCE_BARRIER>{CD3DX12_RESOURCE_BARRIER::UAV(as_buffer->resource.Get())};
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

            raytracing_scene = {std::move(as_buffer)};
        }
    }

    void Renderer::update_lights(entt::registry& registry, const uint32_t frame_idx) {
        registry.view<LightComponent>().each([&](const LightComponent& light) { lights[light.handle.index] = light.light; });

        auto* dst = device->map_buffer(*light_device_buffers[frame_idx]);
        std::memcpy(dst, lights.data(), lights.size() * sizeof(Light));
    }

    std::unique_ptr<rhi::BindGroup> Renderer::bind_resources_for_frame(const uint32_t frame_idx) {
        auto& material_bind_group_builder = device->get_material_bind_group_builder_for_frame(frame_idx);
        material_bind_group_builder.set_buffer("cameras", camera_matrix_buffers->get_device_buffer_for_frame(frame_idx));
        material_bind_group_builder.set_buffer("material_buffer", *material_device_buffers[frame_idx]);
        material_bind_group_builder.set_buffer("lights", *light_device_buffers[frame_idx]);
        material_bind_group_builder.set_buffer("indices", static_mesh_storage->get_index_buffer());
        material_bind_group_builder.set_buffer("vertices", *static_mesh_storage->get_vertex_bindings()[0].buffer);
        material_bind_group_builder.set_buffer("per_frame_data", *per_frame_data_buffers[frame_idx]);
        if(raytracing_scene.buffer) {
            material_bind_group_builder.set_raytracing_scene("raytracing_scene", raytracing_scene);
        }
        material_bind_group_builder.set_image_array("textures", get_texture_array());

        return material_bind_group_builder.build();
    }

    void Renderer::render_shadow_pass(entt::registry& registry, rhi::RenderCommandList& command_list, const rhi::BindGroup& resources) {
        MTR_SCOPE("Renderer", "render_shadows");
        const auto depth_access = rhi::RenderTargetAccess{.begin = {.type = rhi::RenderTargetBeginningAccessType::Clear,
                                                                    .clear_color = glm::vec4{0},
                                                                    .format = rhi::ImageFormat::Depth32},
                                                          .end = {.type = rhi::RenderTargetEndingAccessType::Preserve}};

        command_list.begin_render_pass(*shadow_map_framebuffer, {}, depth_access);

        command_list.bind_pipeline_state(*shadow_pipeline);

        command_list.set_camera_idx(shadow_camera_index);
        command_list.bind_render_resources(resources);

        command_list.bind_mesh_data(*static_mesh_storage);

        {
            registry.view<StaticMeshRenderableComponent>().each([&](const StaticMeshRenderableComponent& mesh_renderable) {
                command_list.set_material_idx(mesh_renderable.material.index);
                command_list.draw(mesh_renderable.mesh.num_indices, mesh_renderable.mesh.first_index);
            });
        }
    }

    void Renderer::draw_objects_in_scene(entt::registry& registry,
                                         const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                         const rhi::BindGroup& material_bind_group) {
        commands->SetGraphicsRootSignature(standard_pipeline->root_signature.Get());
        commands->SetPipelineState(standard_pipeline->pso.Get());

        // Hardcode camera 0 as the player camera
        // TODO: Decide if this is fine
        commands->SetGraphicsRoot32BitConstant(0, 0, 0);

        material_bind_group.bind_to_graphics_signature(commands);
        static_mesh_storage->bind_to_command_list(commands);

        {
            MTR_SCOPE("Renderer", "Render static meshes");
            registry.view<StaticMeshRenderableComponent>().each([&](const StaticMeshRenderableComponent& mesh_renderable) {
                commands->SetGraphicsRoot32BitConstant(0, mesh_renderable.material.index, 1);
                commands->DrawIndexedInstanced(mesh_renderable.mesh.num_indices, 1, mesh_renderable.mesh.first_index, 0, 0);
            });
        }
    }

    void Renderer::bind_scene_framebuffer(const ComPtr<ID3D12GraphicsCommandList4>& commands) const {
        const auto render_target_accesses = std::vector{
            // Scene color
            D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = scene_framebuffer->rtv_handles[0],
                                                 .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                     .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT,
                                                                                              .Color = {0, 0, 0, 0}}}},
                                                 .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}}};

        const auto
            depth_access = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{.cpuDescriptor = *scene_framebuffer->dsv_handle,
                                                                .DepthBeginningAccess =
                                                                    {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                     .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT,
                                                                                              .DepthStencil = {.Depth = 1.0}}}},
                                                                .StencilBeginningAccess = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD,
                                                                .DepthEndingAccess = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE,
                                                                .StencilEndingAccess = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD};

        commands->BeginRenderPass(static_cast<UINT>(render_target_accesses.size()),
                                  render_target_accesses.data(),
                                  &depth_access,
                                  D3D12_RENDER_PASS_FLAG_NONE);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = scene_framebuffer->width;
        viewport.Height = scene_framebuffer->height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(scene_framebuffer->width);
        scissor_rect.bottom = static_cast<LONG>(scene_framebuffer->height);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void Renderer::render_forward_pass(entt::registry& registry,
                                       const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                       const rhi::BindGroup& material_bind_group) {
        bind_scene_framebuffer(commands);

        draw_objects_in_scene(registry, commands, material_bind_group);

        draw_sky(registry, commands);

        commands->EndRenderPass();
    }

    void Renderer::draw_sky(entt::registry& registry, const ComPtr<ID3D12GraphicsCommandList4>& command_list) const {
        const auto atmosphere_view = registry.view<AtmosphericSkyComponent>();
        if(atmosphere_view.size() > 1) {
            logger->error("May only have one atmospheric sky component in a scene");

        } else {
            command_list->SetPipelineState(atmospheric_sky_pipeline->pso.Get());
            command_list->DrawInstanced(3, 1, 0, 0);
        }
    }

    void Renderer::render_backbuffer_output_pass(const ComPtr<ID3D12GraphicsCommandList4>& commands) const {
        const auto* framebuffer = device->get_backbuffer_framebuffer();

        const auto
            render_target_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = framebuffer->rtv_handles[0],
                                                                        .BeginningAccess =
                                                                            {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                             .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                                                      .Color = {0, 0, 0, 0}}}},
                                                                        .EndingAccess = {
                                                                            .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        commands->BeginRenderPass(1, &render_target_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = framebuffer->width;
        viewport.Height = framebuffer->height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(framebuffer->width);
        scissor_rect.bottom = static_cast<LONG>(framebuffer->height);
        commands->RSSetScissorRects(1, &scissor_rect);

        commands->SetGraphicsRoot32BitConstant(0, backbuffer_output_material.index, 1);
        commands->SetPipelineState(backbuffer_output_pipeline->pso.Get());
        commands->DrawInstanced(3, 1, 0, 0);

        commands->EndRenderPass();
    }

    void Renderer::render_denoiser_pass(const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        if(settings.use_optix_denoiser) {
            // TODO: Figure out how to get this to work

            const auto input_layer = OptixImage2D{.data = reinterpret_cast<CUdeviceptr>(mapped_scene_render_target),
                                                  .width = output_framebuffer_size.x,
                                                  .height = output_framebuffer_size.y,
                                                  .rowStrideInBytes = output_framebuffer_size.x * 4 * 4,
                                                  .pixelStrideInBytes = 4 * 4,
                                                  .format = OPTIX_PIXEL_FORMAT_FLOAT4};

            const auto output_layer = OptixImage2D{.data = reinterpret_cast<CUdeviceptr>(mapped_denoised_render_target),
                                                   .width = output_framebuffer_size.x,
                                                   .height = output_framebuffer_size.y,
                                                   .rowStrideInBytes = output_framebuffer_size.x * 4 * 4,
                                                   .pixelStrideInBytes = 4 * 4,
                                                   .format = OPTIX_PIXEL_FORMAT_FLOAT4};

            auto result = optixDenoiserComputeIntensity(optix_denoiser,
                                                        denoiser_stream,
                                                        &input_layer,
                                                        intensity_ptr,
                                                        denoiser_scratch,
                                                        optix_sizes.recommendedScratchSizeInBytes);
            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not compute render target intensity: {}: {}", error_name, error);
                return;
            }

            const auto params = OptixDenoiserParams{.denoiseAlpha = 0, .hdrIntensity = intensity_ptr, .blendFactor = false};

            result = optixDenoiserInvoke(optix_denoiser,
                                         denoiser_stream,
                                         &params,
                                         denoiser_state,
                                         optix_sizes.stateSizeInBytes,
                                         &input_layer,
                                         1,
                                         0,
                                         0,
                                         &output_layer,
                                         denoiser_scratch,
                                         optix_sizes.recommendedScratchSizeInBytes);

            if(result != OPTIX_SUCCESS) {
                const auto* const error_name = optixGetErrorName(result);
                const auto* const error = optixGetErrorString(result);
                logger->error("Could not invoke denoiser: {}: {}", error_name, error);
            }

        } else {
            // No OptiX? Just accumulate rays!

            {
                const auto render_target_access =
                    D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = denoised_framebuffer->rtv_handles[0],
                                                         .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD},
                                                         .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

                commands->BeginRenderPass(1, &render_target_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
            }

            commands->SetPipelineState(accumulation_pipeline->pso.Get());

            commands->SetGraphicsRoot32BitConstant(0, accumulation_material_handle.index, 1);

            const auto& accumulation_image = *all_images[accumulation_target_handle.index];
            {
                const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(accumulation_image.resource.Get(),
                                                                          D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                commands->ResourceBarrier(1, &barrier);
            }

            commands->DrawInstanced(3, 1, 0, 0);

            commands->EndRenderPass();

            const auto& denoised_image = *all_images[denoised_color_target_handle.index];

            {
                const auto barriers = std::vector{CD3DX12_RESOURCE_BARRIER::Transition(accumulation_image.resource.Get(),
                                                                                       D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                                       D3D12_RESOURCE_STATE_COPY_DEST),
                                                  CD3DX12_RESOURCE_BARRIER::Transition(denoised_image.resource.Get(),
                                                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                                       D3D12_RESOURCE_STATE_COPY_SOURCE)};
                commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
            }

            {
                const auto src_copy_location = D3D12_TEXTURE_COPY_LOCATION{.pResource = denoised_image.resource.Get(),
                                                                           .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                                           .SubresourceIndex = 0};

                const auto dst_copy_location = D3D12_TEXTURE_COPY_LOCATION{.pResource = accumulation_image.resource.Get(),
                                                                           .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                                           .SubresourceIndex = 0};

                const auto copy_box = D3D12_BOX{.right = denoised_image.width, .bottom = denoised_image.height, .back = 1};

                commands->CopyTextureRegion(&dst_copy_location, 0, 0, 0, &src_copy_location, &copy_box);
            }

            {
                const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(denoised_image.resource.Get(),
                                                                          D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                                          D3D12_RESOURCE_STATE_RENDER_TARGET);
                commands->ResourceBarrier(1, &barrier);
            }
        }
    }

    void Renderer::render_3d_scene(entt::registry& registry, const ComPtr<ID3D12GraphicsCommandList4>& commands, const uint32_t frame_idx) {
        MTR_SCOPE("Renderer", "render_3d_scene");

        update_lights(registry, frame_idx);

        memcpy(per_frame_data_buffers[frame_idx]->mapped_ptr, &per_frame_data, sizeof(PerFrameData));

        const auto material_bind_group = bind_resources_for_frame(frame_idx);

        // Smol brain v1 - don't use a shadowmap, just cast shadow rays from every ray hit
        // render_shadow_pass(registry, command_list, *material_bind_group);

        render_forward_pass(registry, commands, *material_bind_group);

        render_denoiser_pass(commands);

        render_backbuffer_output_pass(commands);
    }

    void Renderer::create_ui_pipeline() {
        const auto create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "UI Pipeline",
            .vertex_shader = load_shader("ui.vertex"),
            .pixel_shader = load_shader("ui.pixel"),

            .input_assembler_layout = rhi::InputAssemblerLayout::DearImGui,

            .blend_state = rhi::BlendState{.render_target_blends = {rhi::RenderTargetBlendState{.enabled = true}}},

            .rasterizer_state =
                rhi::RasterizerState{
                    .cull_mode = rhi::CullMode::None,
                },

            .depth_stencil_state =
                rhi::DepthStencilState{
                    .enable_depth_test{false},
                    .enable_depth_write{false},
                },

            .render_target_formats = {rhi::ImageFormat::Rgba8},
        };

        ui_pipeline = device->create_render_pipeline_state(create_info);
    }

    void Renderer::create_ui_mesh_buffers() {
        constexpr uint32_t num_ui_vertices = 1 << 20;
        const auto vert_buffer_create_info = rhi::BufferCreateInfo{
            .name = "Dear ImGUI Vertex Buffer",
            .usage = rhi::BufferUsage::VertexBuffer,
            .size = num_ui_vertices * sizeof(ImDrawVert),
        };

        constexpr uint32_t num_ui_indices = 1 << 20;
        const auto index_buffer_create_info = rhi::BufferCreateInfo{
            .name = "Dear ImGUI Index Buffer",
            .usage = rhi::BufferUsage::StagingBuffer,
            .size = num_ui_indices * sizeof(ImDrawIdx),
        };

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            ui_vertex_buffers.push_back(device->create_buffer(vert_buffer_create_info));
            ui_index_buffers.push_back(device->create_buffer(index_buffer_create_info));
        }
    }

    void Renderer::render_ui(const ComPtr<ID3D12GraphicsCommandList4>& commands, const uint32_t frame_idx) const {
        MTR_SCOPE("Renderer", "render_ui");

        // TODO: Instead of allocating and destroying buffers every frame, make a couple large buffers for the UI mesh data to live in

        ImDrawData* draw_data = ImGui::GetDrawData();
        if(draw_data == nullptr) {
            return;
        }

        commands->SetPipelineState(ui_pipeline->pso.Get());

        {
            const auto viewport = D3D12_VIEWPORT{.TopLeftX = draw_data->DisplayPos.x,
                                                 .TopLeftY = draw_data->DisplayPos.y,
                                                 .Width = draw_data->DisplaySize.x,
                                                 .Height = draw_data->DisplaySize.y,
                                                 .MinDepth = 0,
                                                 .MaxDepth = 1};
            commands->RSSetViewports(1, &viewport);
        }

        uint32_t vertex_offset{0};
        uint32_t index_offset{0};

        for(int32_t i = 0; i < draw_data->CmdListsCount; i++) {
            const auto* cmd_list = draw_data->CmdLists[i];
            const auto* imgui_vertices = cmd_list->VtxBuffer.Data;
            const auto* imgui_indices = cmd_list->IdxBuffer.Data;

            const auto vertex_buffer_size = static_cast<uint32_t>(cmd_list->VtxBuffer.size_in_bytes());
            const auto index_buffer_size = static_cast<uint32_t>(cmd_list->IdxBuffer.size_in_bytes());

            const auto vert_buffer_create_info = rhi::BufferCreateInfo{
                .name = "Dear ImGUI Vertex Buffer",
                .usage = rhi::BufferUsage::StagingBuffer,
                .size = vertex_buffer_size,
            };
            auto vertex_buffer = device->create_buffer(vert_buffer_create_info);
            memcpy(vertex_buffer->mapped_ptr, imgui_vertices, vertex_buffer_size);

            const auto index_buffer_create_info = rhi::BufferCreateInfo{
                .name = "Dear ImGUI Index Buffer",
                .usage = rhi::BufferUsage::StagingBuffer,
                .size = index_buffer_size,
            };
            auto index_buffer = device->create_buffer(index_buffer_create_info);
            memcpy(index_buffer->mapped_ptr, imgui_indices, index_buffer_size);

            {
                const auto vb_view = D3D12_VERTEX_BUFFER_VIEW{.BufferLocation = vertex_buffer->resource->GetGPUVirtualAddress(),
                                                              .SizeInBytes = vertex_buffer->size,
                                                              .StrideInBytes = sizeof(ImDrawVert)};
                commands->IASetVertexBuffers(0, 1, &vb_view);

                const auto ib_view = D3D12_INDEX_BUFFER_VIEW{.BufferLocation = index_buffer->resource->GetGPUVirtualAddress(),
                                                             .SizeInBytes = index_buffer->size,
                                                             .Format = DXGI_FORMAT_R32_UINT};
                commands->IASetIndexBuffer(&ib_view);

                commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            }

            for(const auto& cmd : cmd_list->CmdBuffer) {
                if(cmd.UserCallback) {
                    cmd.UserCallback(cmd_list, &cmd);

                } else {
                    const auto imgui_material_idx = reinterpret_cast<uint64_t>(cmd.TextureId);
                    const auto material_idx = static_cast<uint32_t>(imgui_material_idx);
                    commands->SetGraphicsRoot32BitConstant(0, material_idx, 1);

                    {
                        const auto& clip_rect = cmd.ClipRect;
                        const auto pos = draw_data->DisplayPos;
                        const auto top_left_x = clip_rect.x - pos.x;
                        const auto top_left_y = clip_rect.y - pos.y;
                        const auto width = clip_rect.z - pos.x;
                        const auto height = clip_rect.w - pos.y;
                        const auto rect = D3D12_RECT{.left = static_cast<LONG>(top_left_x),
                                                     .top = static_cast<LONG>(top_left_y),
                                                     .right = static_cast<LONG>(top_left_x + width),
                                                     .bottom = static_cast<LONG>(top_left_y + height)};
                        commands->RSSetScissorRects(1, &rect);
                    }

                    commands->DrawIndexedInstanced(cmd.ElemCount, 1, cmd.IdxOffset, 0, 0);
                }
            }

            device->schedule_buffer_destruction(std::move(vertex_buffer));
            device->schedule_buffer_destruction(std::move(index_buffer));
        }
    }
} // namespace renderer

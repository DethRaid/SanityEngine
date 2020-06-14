#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <entt/entity/registry.hpp>
#include <ftl/atomic_counter.h>
#include <imgui/imgui.h>
#include <minitrace.h>
#include <rx/core/log.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "core/align.hpp"
#include "core/components.hpp"
#include "core/constants.hpp"
#include "core/defer.hpp"
#include "core/errors.hpp"
#include "loading/image_loading.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/camera_matrix_buffer.hpp"
#include "renderer/render_components.hpp"
#include "rhi/d3dx12.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_device.hpp"
#include "sanity_engine.hpp"

namespace renderer {

    constexpr uint32_t MATERIAL_DATA_BUFFER_SIZE = 1 << 20;

#pragma region Cube
    Rx::Vector<StandardVertex> Renderer::cube_vertices = Rx::Array{
        // Front
        StandardVertex{.position = {-0.5f, 0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, -0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, -0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, 0.5f, 0.5f}, .normal = {0, 0, 1}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Right
        StandardVertex{.position = {-0.5f, -0.5f, -0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, 0.5f, 0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, -0.5f, 0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, 0.5f, -0.5f}, .normal = {-1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Left
        StandardVertex{.position = {0.5f, 0.5f, 0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, -0.5f, -0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, -0.5f, 0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, 0.5f, -0.5f}, .normal = {1, 0, 0}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Back
        StandardVertex{.position = {0.5f, 0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, -0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, -0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, 0.5f, -0.5f}, .normal = {0, 0, -1}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Top
        StandardVertex{.position = {-0.5f, 0.5f, -0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, 0.5f, 0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, 0.5f, -0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, 0.5f, 0.5f}, .normal = {0, 1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},

        // Bottom
        StandardVertex{.position = {0.5f, -0.5f, 0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, -0.5f, -0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {0.5f, -0.5f, -0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
        StandardVertex{.position = {-0.5f, -0.5f, 0.5f}, .normal = {0, -1, 0}, .color = 0xFFCDCDCD, .texcoord = {}},
    };

    Rx::Vector<uint32_t> Renderer::cube_indices = Rx::Array{
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

    RX_LOG("Renderer", logger);

    Renderer::Renderer(GLFWwindow* window, const Settings& settings_in)
        : start_time{std::chrono::high_resolution_clock::now()},
          settings{settings_in},
          device{make_render_device(rhi::RenderBackend::D3D12, window, settings_in)},
          camera_matrix_buffers{std::make_unique<CameraMatrixBuffer>(*device, settings_in.num_in_flight_gpu_frames)} {
        MTR_SCOPE("Renderer", "Renderer");

        // logger->set_level(spdlog::level::debug);

        RX_ASSERT(settings.render_scale > 0, "Render scale may not be 0 or less");

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        output_framebuffer_size = {static_cast<uint32_t>(width * settings.render_scale),
                                   static_cast<uint32_t>(height * settings.render_scale)};

        create_static_mesh_storage();

        create_per_frame_data_buffers();

        create_material_data_buffers();

        create_ui_pipeline();

        create_ui_mesh_buffers();

        create_light_buffers();

        create_builtin_images();

        forward_pass = std::make_unique<ForwardPass>(*this, output_framebuffer_size);
        denoiser_pass = std::make_unique<DenoiserPass>(*this, output_framebuffer_size, *forward_pass);
        backbuffer_output_pass = std::make_unique<BackbufferOutputPass>(*this, *denoiser_pass);
    }

    void Renderer::load_noise_texture(const Rx::String& filepath) {
        auto args = LoadImageToGpuArgs{.texture_name_in = filepath, .renderer_in = this};
        ftl::AtomicCounter counter{task_scheduler.get()};
        task_scheduler->AddTask({load_image_to_gpu, &args}, &counter);

        task_scheduler->WaitForCounter(&counter, 0, true);

        if(!args.handle_out) {
            logger->error("Could not load noise texture %s", filepath);
            return;
        }

        noise_texture_handle = *args.handle_out;
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
        render_3d_scene(registry, command_list.Get(), frame_idx);

        // At the end of render_3d_scene, the backbuffer framebuffer is bound. render_ui thus simply renders directly onto the backbuffer
        render_ui(command_list, frame_idx);

        device->submit_command_list(std::move(command_list));
    }

    void Renderer::end_frame() const { device->end_frame(); }

    void Renderer::add_raytracing_objects_to_scene(const Rx::Vector<rhi::RaytracingObject>& new_objects) {
        raytracing_objects += new_objects;
        raytracing_scene_dirty = true;
    }

    TextureHandle Renderer::create_image(const rhi::ImageCreateInfo& create_info) {
        const auto idx = static_cast<uint32_t>(all_images.size());

        all_images.push_back(device->create_image(create_info));
        image_name_to_index.insert(create_info.name, idx);

        logger->verbose("Created texture %s with index %u", create_info.name, idx);

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
            logger->error("Could not upload texture data");
        }

        return handle;
    }

    std::optional<TextureHandle> Renderer::get_image_handle(const Rx::String& name) {
        if(const auto* idx = image_name_to_index.find(name)) {
            return TextureHandle{*idx};

        } else {
            return std::nullopt;
        }
    }

    rhi::Image& Renderer::get_image(const Rx::String& image_name) const {
        if(const auto* idx = image_name_to_index.find(image_name)) {
            return *all_images[*idx];

        } else {
            const auto message = fmt::format("Image '{}' does not exist", image_name.data());
            throw std::exception(message.c_str());
        }
    }

    rhi::Image& Renderer::get_image(const TextureHandle handle) const { return *all_images[handle.index]; }

    void Renderer::schedule_texture_destruction(const TextureHandle& image_handle) {
        auto image = std::move(all_images[image_handle.index]);
        device->schedule_image_destruction(std::move(image));
    }

    StandardMaterialHandle Renderer::allocate_standard_material(const StandardMaterial& material) {
        if(!free_material_handles.is_empty()) {
            const auto& handle = free_material_handles.last();
            free_material_handles.pop_back();
            standard_materials[handle.index] = material;
            return handle;
        }

        const auto handle = StandardMaterialHandle{.index = static_cast<uint32_t>(standard_materials.size())};
        standard_materials.push_back(material);

        return handle;
    }

    rhi::Buffer& Renderer::get_standard_material_buffer_for_frame(const uint32_t frame_idx) const {
        return *material_device_buffers[frame_idx];
    }

    void Renderer::deallocate_standard_material(const StandardMaterialHandle handle) { free_material_handles.push_back(handle); }

    rhi::RenderDevice& Renderer::get_render_device() const { return *device; }

    rhi::MeshDataStore& Renderer::get_static_mesh_store() const { return *static_mesh_storage; }

    void Renderer::begin_device_capture() const { device->begin_capture(); }

    void Renderer::end_device_capture() const { device->end_capture(); }

    TextureHandle Renderer::get_noise_texture() const { return noise_texture_handle; }

    TextureHandle Renderer::get_pink_texture() const { return pink_texture_handle; }

    TextureHandle Renderer::get_default_normal_roughness_texture() const { return normal_roughness_texture_handle; }

    TextureHandle Renderer::get_default_specular_color_emission_texture() const { return specular_emission_texture_handle; }

    RaytracableGeometryHandle Renderer::create_raytracing_geometry(const rhi::Buffer& vertex_buffer,
                                                                   const rhi::Buffer& index_buffer,
                                                                   const Rx::Vector<rhi::Mesh>& meshes,
                                                                   const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        auto new_ray_geo = build_acceleration_structure_for_meshes(commands, *device, vertex_buffer, index_buffer, meshes);

        const auto handle_idx = static_cast<uint32_t>(raytracing_geometries.size());
        raytracing_geometries.push_back(std::move(new_ray_geo));

        return {handle_idx};
    }

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
            create_info.name = fmt::format("Per frame data buffer {}", i).c_str();
            per_frame_data_buffers.push_back(device->create_buffer(create_info));
        }
    }

    void Renderer::create_material_data_buffers() {
        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer, .size = MATERIAL_DATA_BUFFER_SIZE};
        material_device_buffers.reserve(settings.num_in_flight_gpu_frames);
        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            create_info.name = fmt::format("Material Data Buffer {}", 1).c_str();
            material_device_buffers.push_back(device->create_buffer(create_info));
        }
    }

    void Renderer::create_light_buffers() {
        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer, .size = MAX_NUM_LIGHTS * sizeof(Light)};

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            create_info.name = fmt::format("Light Buffer {}", i).c_str();
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

            auto pink_texture_pixel = Rx::Vector<uint32_t>{};
            pink_texture_pixel.reserve(64);
            for(uint32_t i = 0; i < 64; i++) {
                pink_texture_pixel.push_back(0xFFFF00FF);
            }

            pink_texture_handle = create_image(pink_texture_create_info, pink_texture_pixel.data(), commands);
        }

        {
            const auto normal_roughness_texture_create_info = rhi::ImageCreateInfo{.name = "Default Normal/Roughness",
                                                                                   .usage = rhi::ImageUsage::SampledImage,
                                                                                   .format = rhi::ImageFormat::Rgba8,
                                                                                   .width = 8,
                                                                                   .height = 8};

            auto normal_roughness_texture_pixel = Rx::Vector<uint32_t>{};
            normal_roughness_texture_pixel.reserve(64);
            for(uint32_t i = 0; i < 64; i++) {
                normal_roughness_texture_pixel.push_back(0x80FF8080);
            }

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

            auto specular_emission_texture_pixel = Rx::Vector<uint32_t>{};
            specular_emission_texture_pixel.reserve(64);
            for(uint32_t i = 0; i < 64; i++) {
                specular_emission_texture_pixel.push_back(0x00373737);
            }

            specular_emission_texture_handle = create_image(specular_emission_texture_create_info,
                                                            specular_emission_texture_pixel.data(),
                                                            commands);
        }

        device->submit_command_list(commands);
    }

    Rx::Vector<const rhi::Image*> Renderer::get_texture_array() const {
        Rx::Vector<const rhi::Image*> images;
        images.reserve(all_images.size());

        all_images.each_fwd([&](const std::unique_ptr<rhi::Image>& image) { images.push_back(image.get()); });

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
        memcpy(buffer.mapped_ptr, standard_materials.data(), standard_materials.size());
    }

    void Renderer::rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        // TODO: figure out how to update the raytracing scene without needing a full rebuild

        if(raytracing_scene.buffer) {
            device->schedule_buffer_destruction(std::move(raytracing_scene.buffer));
        }

        if(!raytracing_objects.is_empty()) {
            constexpr auto max_num_objects = UINT32_MAX / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

            RX_ASSERT(raytracing_objects.size() < max_num_objects, "May not have more than %u objects because uint32", max_num_objects);

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

                desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

                desc.InstanceContributionToHitGroupIndex = object.material.handle;

                const auto& ray_geo = raytracing_geometries[object.geometry_handle.index];

                const auto& buffer = static_cast<const rhi::Buffer&>(*ray_geo.blas_buffer);
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

            const Rx::Vector<D3D12_RESOURCE_BARRIER> barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::UAV(as_buffer->resource.Get())};
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

            raytracing_scene = {std::move(as_buffer)};
        }
    }

    void Renderer::update_lights(entt::registry& registry, const uint32_t frame_idx) {
        registry.view<LightComponent>().each([&](const LightComponent& light) { lights[light.handle.index] = light.light; });

        auto* dst = device->map_buffer(*light_device_buffers[frame_idx]);
        std::memcpy(dst, lights.data(), lights.size() * sizeof(Light));
    }

    std::unique_ptr<rhi::BindGroup> Renderer::bind_global_resources_for_frame(const uint32_t frame_idx) {
        auto& material_bind_group_builder = device->get_material_bind_group_builder_for_frame(frame_idx);
        material_bind_group_builder.set_buffer("cameras", camera_matrix_buffers->get_device_buffer_for_frame(frame_idx));
        material_bind_group_builder.set_buffer("lights", *light_device_buffers[frame_idx]);
        material_bind_group_builder.set_buffer("per_frame_data", *per_frame_data_buffers[frame_idx]);
        material_bind_group_builder.set_image_array("textures", get_texture_array());

        // TODO: We may have to rethink raytracing if we start using multiple buffers for mesh data storage
        material_bind_group_builder.set_buffer("indices", static_mesh_storage->get_index_buffer());
        material_bind_group_builder.set_buffer("vertices", *static_mesh_storage->get_vertex_bindings()[0].buffer);
        if(raytracing_scene.buffer) {
            material_bind_group_builder.set_raytracing_scene("raytracing_scene", raytracing_scene);
        }

        return material_bind_group_builder.build();
    }

    void Renderer::render_3d_scene(entt::registry& registry, ID3D12GraphicsCommandList4* commands, const uint32_t frame_idx) {
        MTR_SCOPE("Renderer", "render_3d_scene");

        update_lights(registry, frame_idx);

        memcpy(per_frame_data_buffers[frame_idx]->mapped_ptr, &per_frame_data, sizeof(PerFrameData));

        forward_pass->execute(commands, registry, frame_idx);

        denoiser_pass->execute(commands, registry, frame_idx);

        backbuffer_output_pass->execute(commands, registry, frame_idx);
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

            .render_target_formats = Rx::Array{rhi::ImageFormat::Rgba8},
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

        {
            const auto* backbuffer_framebuffer = device->get_backbuffer_framebuffer();

            const auto
                backbuffer_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = backbuffer_framebuffer->rtv_handles[0],
                                                                         .BeginningAccess =
                                                                             {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE},
                                                                         .EndingAccess = {
                                                                             .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

            commands->BeginRenderPass(1, &backbuffer_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
        }

        // TODO: Instead of allocating and destroying buffers every frame, make a couple large buffers for the UI mesh data to live in

        ImDrawData* draw_data = ImGui::GetDrawData();
        if(draw_data == nullptr) {
            commands->EndRenderPass();
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
                    const auto texture_idx = static_cast<uint32_t>(imgui_material_idx);
                    commands->SetGraphicsRoot32BitConstant(0, texture_idx, 1);

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

        commands->EndRenderPass();
    }
} // namespace renderer

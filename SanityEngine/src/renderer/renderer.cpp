#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <entt/entity/registry.hpp>
#include <ftl/atomic_counter.h>
#include <imgui/imgui.h>
#include <rx/console/variable.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>
#include <winrt/Windows.System.Threading.h>

#include "core/align.hpp"
#include "core/components.hpp"
#include "core/constants.hpp"
#include "core/defer.hpp"
#include "core/errors.hpp"
#include "loading/image_loading.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/camera_matrix_buffer.hpp"
#include "renderer/render_components.hpp"
#include "renderpasses/backbuffer_output_pass.hpp"
#include "renderpasses/ui_render_pass.hpp"
#include "rhi/d3dx12.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_device.hpp"
#include "sanity_engine.hpp"

using winrt::Windows::System::Threading::ThreadPool;

namespace renderer {

    constexpr Uint32 MATERIAL_DATA_BUFFER_SIZE = 1 << 20;

    RX_LOG("Renderer", logger);

    RX_CONSOLE_IVAR(r_max_drawcalls_per_frame,
                    "render.MaxDrawcallsPerFrame",
                    "Maximum number of drawcalls that may be issued in a given frame",
                    1,
                    INT_MAX,
                    100000);

    Renderer::Renderer(GLFWwindow* window, // NOLINT(cppcoreguidelines-pro-type-member-init)
                       const Settings& settings_in)
        : start_time{std::chrono::high_resolution_clock::now()},
          settings{settings_in},
          device{make_render_device(window, settings_in)},
          camera_matrix_buffers{Rx::make_ptr<CameraMatrixBuffer>(RX_SYSTEM_ALLOCATOR, *device)} {
        ZoneScoped;

        RX_ASSERT(settings.render_scale > 0, "Render scale may not be 0 or less");

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        output_framebuffer_size = {static_cast<Uint32>(width * settings.render_scale), static_cast<Uint32>(height * settings.render_scale)};

        create_static_mesh_storage();

        create_per_frame_buffers();

        create_material_data_buffers();

        create_light_buffers();

        create_builtin_images();

        create_render_passes();
    }

    void Renderer::begin_frame(const uint64_t frame_count) {
        device->begin_frame(frame_count);

        const auto cur_time = std::chrono::high_resolution_clock::now();
        const auto duration_since_start = cur_time - start_time;
        const auto ns_since_start = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_start).count();
        const auto time_since_start = static_cast<double>(ns_since_start) / 1000000000.0;

        per_frame_data.time_since_start = static_cast<Float32>(time_since_start);

        const auto frame_idx = device->get_cur_gpu_frame_idx();
        next_unused_model_matrix_per_frame[frame_idx]->store(0);
    }

    void Renderer::render_all(SynchronizedResourceAccessor<entt::registry>& registry, const World& world) {
        ZoneScoped;

        const auto frame_idx = device->get_cur_gpu_frame_idx();

        auto command_list = device->create_command_list(frame_idx);
        command_list->SetName(L"Main Render Command List");

        {
            TracyD3D12Zone(RenderDevice::tracy_context, command_list.Get(), "Renderer::render_all");
            PIXScopedEvent(command_list.Get(), PIX_COLOR_DEFAULT, "Renderer::render_all");
            if(raytracing_scene_dirty) {
                rebuild_raytracing_scene(command_list);
                raytracing_scene_dirty = false;
            }

            update_cameras(*registry, frame_idx);

            upload_material_data(frame_idx);

            update_lights(*registry, frame_idx);

            {
                ZoneScopedN("Renderer::update_per_frame_data");
                memcpy(per_frame_data_buffers[frame_idx]->mapped_ptr, &per_frame_data, sizeof(PerFrameData));
            }

            {
                ZoneScopedN("Renderer::render_passes");

                TracyD3D12Zone(RenderDevice::tracy_context, command_list.Get(), "Renderer::render_passes");
                PIXScopedEvent(command_list.Get(), PIX_COLOR_DEFAULT, "Renderer::render_passes");

                render_passes.each_fwd(
                    [&](Rx::Ptr<RenderPass>& render_pass) { render_pass->render(command_list.Get(), *registry, frame_idx, world); });
            }
        }

        device->submit_command_list(Rx::Utility::move(command_list));
    }

    void Renderer::end_frame() const { device->end_frame(); }

    void Renderer::add_raytracing_objects_to_scene(const Rx::Vector<RaytracingObject>& new_objects) {
        raytracing_objects.append(new_objects);
        raytracing_scene_dirty = true;
    }

    TextureHandle Renderer::create_image(const ImageCreateInfo& create_info) {
        const auto idx = static_cast<Uint32>(all_images.size());

        auto image = device->create_image(create_info);
        if(image) {
            all_images.push_back(Rx::Utility::move(image));
            image_name_to_index.insert(create_info.name, idx);

            logger->verbose("Created texture %s with index %u", create_info.name, idx);

            return {idx};
        } else {
            return pink_texture_handle;
        }
    }

    TextureHandle Renderer::create_image(const ImageCreateInfo& create_info,
                                         const void* image_data,
                                         const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        ZoneScoped;

        const auto scope_name = Rx::String::format("create_image(\"%s\")", create_info.name);
        TracyD3D12Zone(RenderDevice::tracy_context, commands.Get(), scope_name.data());
        PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, scope_name.data());

        const auto handle = create_image(create_info);
        auto& image = *all_images[handle.index];

        if(create_info.usage == ImageUsage::UnorderedAccess) {
            // Transition the image to COPY_DEST
            const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(image.resource.Get(),
                                                                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                                 D3D12_RESOURCE_STATE_COPY_DEST)};

            commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
        }

        const auto num_bytes_in_texture = create_info.width * create_info.height * size_in_bytes(create_info.format);

        const auto& staging_buffer = device->get_staging_buffer(num_bytes_in_texture);

        const auto subresource = D3D12_SUBRESOURCE_DATA{
            .pData = image_data,
            .RowPitch = create_info.width * 4,
            .SlicePitch = create_info.width * create_info.height * 4,
        };

        const auto result = UpdateSubresources(commands.Get(), image.resource.Get(), staging_buffer.resource.Get(), 0, 0, 1, &subresource);
        if(result == 0 || FAILED(result)) {
            logger->error("Could not upload texture data");

            return pink_texture_handle;
        } else {
            if(create_info.usage == ImageUsage::UnorderedAccess) {
                // Transition the image back to UNORDERED_ACCESS
                const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(image.resource.Get(),
                                                                                     D3D12_RESOURCE_STATE_COPY_DEST,
                                                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS)};

                commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
            }
        }

        return handle;
    }

    Rx::Optional<TextureHandle> Renderer::get_image_handle(const Rx::String& name) {
        if(const auto* idx = image_name_to_index.find(name)) { // NOLINT(bugprone-branch-clone)
            return TextureHandle{*idx};

        } else {
            return Rx::nullopt;
        }
    }

    Image& Renderer::get_image(const Rx::String& image_name) const {
        if(const auto* idx = image_name_to_index.find(image_name)) {
            return *all_images[*idx];

        } else {
            Rx::abort("Image '%s' does not exist", image_name);
        }
    }

    Image& Renderer::get_image(const TextureHandle handle) const {
        ZoneScoped;

        return *all_images[handle.index];
    }

    void Renderer::schedule_texture_destruction(const TextureHandle& image_handle) {
        auto image = Rx::Utility::move(all_images[image_handle.index]);
        device->schedule_image_destruction(Rx::Utility::move(image));
    }

    StandardMaterialHandle Renderer::allocate_standard_material(const StandardMaterial& material) {
        if(!free_material_handles.is_empty()) {
            const auto& handle = free_material_handles.last();
            free_material_handles.pop_back();
            standard_materials[handle.index] = material;
            return handle;
        }

        const auto handle = StandardMaterialHandle{.index = static_cast<Uint32>(standard_materials.size())};
        standard_materials.push_back(material);

        return handle;
    }

    Buffer& Renderer::get_standard_material_buffer_for_frame(const Uint32 frame_idx) const { return *material_device_buffers[frame_idx]; }

    void Renderer::deallocate_standard_material(const StandardMaterialHandle handle) { free_material_handles.push_back(handle); }

    RenderDevice& Renderer::get_render_device() const { return *device; }

    MeshDataStore& Renderer::get_static_mesh_store() const { return *static_mesh_storage; }

    void Renderer::begin_device_capture() const { device->begin_capture(); }

    void Renderer::end_device_capture() const { device->end_capture(); }

    TextureHandle Renderer::get_noise_texture() const { return noise_texture_handle; }

    TextureHandle Renderer::get_pink_texture() const { return pink_texture_handle; }

    TextureHandle Renderer::get_default_normal_roughness_texture() const { return normal_roughness_texture_handle; }

    TextureHandle Renderer::get_default_specular_color_emission_texture() const { return specular_emission_texture_handle; }

    RaytracableGeometryHandle Renderer::create_raytracing_geometry(const Buffer& vertex_buffer,
                                                                   const Buffer& index_buffer,
                                                                   const Rx::Vector<Mesh>& meshes,
                                                                   const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        TracyD3D12Zone(RenderDevice::tracy_context, commands.Get(), "Renderer::create_raytracing_geometry");
        PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, "Renderer::create_raytracing_geometry");

        auto new_ray_geo = build_acceleration_structure_for_meshes(commands, *device, vertex_buffer, index_buffer, meshes);

        const auto handle_idx = static_cast<Uint32>(raytracing_geometries.size());
        raytracing_geometries.push_back(Rx::Utility::move(new_ray_geo));

        return {handle_idx};
    }

    void Renderer::create_static_mesh_storage() {
        const auto vertex_create_info = BufferCreateInfo{
            .name = "Static Mesh Vertex Buffer",
            .usage = BufferUsage::VertexBuffer,
            .size = STATIC_MESH_VERTEX_BUFFER_SIZE,
        };

        auto vertex_buffer = device->create_buffer(vertex_create_info);

        const auto index_buffer_create_info = BufferCreateInfo{.name = "Static Mesh Index Buffer",
                                                               .usage = BufferUsage::IndexBuffer,
                                                               .size = STATIC_MESH_INDEX_BUFFER_SIZE};

        auto index_buffer = device->create_buffer(index_buffer_create_info);

        static_mesh_storage = Rx::make_ptr<MeshDataStore>(RX_SYSTEM_ALLOCATOR,
                                                          *device,
                                                          Rx::Utility::move(vertex_buffer),
                                                          Rx::Utility::move(index_buffer));
    }

    void Renderer::create_per_frame_buffers() {
        ZoneScoped;

        const auto num_gpu_frames = device->get_max_num_gpu_frames();

        per_frame_data_buffers.reserve(num_gpu_frames);
        model_matrix_buffers.reserve(num_gpu_frames);

        auto per_frame_data_buffer_create_info = BufferCreateInfo{
            .usage = BufferUsage::ConstantBuffer,
            .size = sizeof(PerFrameData),
        };

        auto model_matrix_buffer_create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer,
                                                                .size = static_cast<Uint32>(sizeof(glm::mat4) *
                                                                                            r_max_drawcalls_per_frame->get())};

        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            per_frame_data_buffer_create_info.name = Rx::String::format("Per frame data buffer %d", i);
            per_frame_data_buffers.push_back(device->create_buffer(per_frame_data_buffer_create_info));

            model_matrix_buffer_create_info.name = Rx::String::format("Model matrix buffer %d", i);
            model_matrix_buffers.push_back(device->create_buffer(model_matrix_buffer_create_info));

            next_unused_model_matrix_per_frame.emplace_back(Rx::make_ptr<Rx::Concurrency::Atomic<Uint32>>(RX_SYSTEM_ALLOCATOR, 0_u32));
        }
    }

    void Renderer::create_material_data_buffers() {
        ZoneScoped;

        const auto num_gpu_frames = device->get_max_num_gpu_frames();

        auto create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer, .size = MATERIAL_DATA_BUFFER_SIZE};
        material_device_buffers.reserve(num_gpu_frames);
        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            create_info.name = Rx::String::format("Material Data Buffer %d", i);
            material_device_buffers.push_back(device->create_buffer(create_info));
        }
    }

    void Renderer::create_light_buffers() {
        ZoneScoped;

        const auto num_gpu_frames = device->get_max_num_gpu_frames();

        auto create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer, .size = MAX_NUM_LIGHTS * sizeof(Light)};

        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            create_info.name = Rx::String::format("Light Buffer %d", i);
            light_device_buffers.push_back(device->create_buffer(create_info));
        }
    }

    void Renderer::create_builtin_images() {
        ZoneScoped;

        load_noise_texture("data/textures/LDR_RGBA_0.png");

        auto commands = device->create_command_list();
        commands->SetName(L"Renderer::create_builtin_images");

        {
            TracyD3D12Zone(RenderDevice::tracy_context, commands.Get(), "Renderer::create_builtin_images");
            PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, "Renderer::create_builtin_images");

            {
                const auto pink_texture_create_info = ImageCreateInfo{.name = "Pink",
                                                                      .usage = ImageUsage::SampledImage,
                                                                      .format = ImageFormat::Rgba8,
                                                                      .width = 8,
                                                                      .height = 8};

                auto pink_texture_pixel = Rx::Vector<Uint32>{};
                pink_texture_pixel.reserve(64);
                for(Uint32 i = 0; i < 64; i++) {
                    pink_texture_pixel.push_back(0xFFFF00FF);
                }

                pink_texture_handle = create_image(pink_texture_create_info, pink_texture_pixel.data(), commands);
            }

            {
                const auto normal_roughness_texture_create_info = ImageCreateInfo{.name = "Default Normal/Roughness",
                                                                                  .usage = ImageUsage::SampledImage,
                                                                                  .format = ImageFormat::Rgba8,
                                                                                  .width = 8,
                                                                                  .height = 8};

                auto normal_roughness_texture_pixel = Rx::Vector<Uint32>{};
                normal_roughness_texture_pixel.reserve(64);
                for(Uint32 i = 0; i < 64; i++) {
                    normal_roughness_texture_pixel.push_back(0x80FF8080);
                }

                normal_roughness_texture_handle = create_image(normal_roughness_texture_create_info,
                                                               normal_roughness_texture_pixel.data(),
                                                               commands);
            }

            {
                const auto specular_emission_texture_create_info = ImageCreateInfo{.name = "Default Specular Color/Emission",
                                                                                   .usage = ImageUsage::SampledImage,
                                                                                   .format = ImageFormat::Rgba8,
                                                                                   .width = 8,
                                                                                   .height = 8};

                auto specular_emission_texture_pixel = Rx::Vector<Uint32>{};
                specular_emission_texture_pixel.reserve(64);
                for(Uint32 i = 0; i < 64; i++) {
                    specular_emission_texture_pixel.push_back(0x00373737);
                }

                specular_emission_texture_handle = create_image(specular_emission_texture_create_info,
                                                                specular_emission_texture_pixel.data(),
                                                                commands);
            }
        }

        device->submit_command_list(Rx::Utility::move(commands));
    }

    void Renderer::load_noise_texture(const Rx::String& filepath) {
        ZoneScoped;

        const auto handle = load_image_to_gpu(filepath, *this);

        if(!handle) {
            logger->error("Could not load noise texture %s", filepath);
            return;
        }

        noise_texture_handle = *handle;
    }

    void Renderer::create_render_passes() {
        render_passes.reserve(4);
        render_passes.push_back(Rx::make_ptr<ForwardPass>(RX_SYSTEM_ALLOCATOR, *this, output_framebuffer_size));
        render_passes.push_back(
            Rx::make_ptr<DenoiserPass>(RX_SYSTEM_ALLOCATOR, *this, output_framebuffer_size, dynamic_cast<ForwardPass&>(*render_passes[0])));
        render_passes.push_back(
            Rx::make_ptr<BackbufferOutputPass>(RX_SYSTEM_ALLOCATOR, *this, dynamic_cast<DenoiserPass&>(*render_passes[1])));
        render_passes.push_back(Rx::make_ptr<UiPass>(RX_SYSTEM_ALLOCATOR, *this));
    }

    Rx::Vector<const Image*> Renderer::get_texture_array() const {
        ZoneScoped;

        Rx::Vector<const Image*> images;
        images.reserve(all_images.size());

        all_images.each_fwd([&](const Rx::Ptr<Image>& image) { images.push_back(image.get()); });

        return images;
    }

    void Renderer::update_cameras(entt::registry& registry, const Uint32 frame_idx) const {
        ZoneScoped;

        registry.view<TransformComponent, CameraComponent>().each([&](const TransformComponent& transform, const CameraComponent& camera) {
            auto& matrices = camera_matrix_buffers->get_camera_matrices(camera.idx);

            matrices.copy_matrices_to_previous();
            matrices.calculate_view_matrix(transform);
            matrices.calculate_projection_matrix(camera);
        });

        camera_matrix_buffers->upload_data(frame_idx);
    }

    void Renderer::upload_material_data(const Uint32 frame_idx) {
        ZoneScoped;

        auto& buffer = *material_device_buffers[frame_idx];
        memcpy(buffer.mapped_ptr, standard_materials.data(), standard_materials.size());
    }

    void Renderer::rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        TracyD3D12Zone(RenderDevice::tracy_context, commands.Get(), "RebuildRaytracingScene");
        PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, "Renderer::rebuild_raytracing_scene");

        // TODO: figure out how to update the raytracing scene without needing a full rebuild

        if(raytracing_scene.buffer) {
            device->schedule_buffer_destruction(Rx::Utility::move(raytracing_scene.buffer));
        }

        if(!raytracing_objects.is_empty()) {
            constexpr auto max_num_objects = UINT32_MAX / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

            RX_ASSERT(raytracing_objects.size() < max_num_objects, "May not have more than %u objects because uint32", max_num_objects);

            const auto instance_buffer_size = static_cast<Uint32>(raytracing_objects.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
            auto instance_buffer = device->get_staging_buffer(instance_buffer_size);
            auto* instance_buffer_array = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instance_buffer.mapped_ptr);

            for(Uint32 i = 0; i < raytracing_objects.size(); i++) {
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

                const auto& buffer = static_cast<const Buffer&>(*ray_geo.blas_buffer);
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

            auto scratch_buffer = device->get_scratch_buffer(static_cast<Uint32>(prebuild_info.ScratchDataSizeInBytes));

            const auto as_buffer_create_info = BufferCreateInfo{.name = "Raytracing Scene",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<Uint32>(prebuild_info.ResultDataMaxSizeInBytes)};
            auto as_buffer = device->create_buffer(as_buffer_create_info);

            const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
                .DestAccelerationStructureData = as_buffer->resource->GetGPUVirtualAddress(),
                .Inputs = as_inputs,
                .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress(),
            };

            DEFER(a, [&]() { device->return_staging_buffer(Rx::Utility::move(instance_buffer)); });
            DEFER(b, [&]() { device->return_scratch_buffer(Rx::Utility::move(scratch_buffer)); });

            commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

            const Rx::Vector<D3D12_RESOURCE_BARRIER> barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::UAV(as_buffer->resource.Get())};
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

            raytracing_scene = {Rx::Utility::move(as_buffer)};
        }
    }

    void Renderer::update_lights(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        registry.view<LightComponent>().each([&](const LightComponent& light) { lights[light.handle.index] = light.light; });

        auto* dst = device->map_buffer(*light_device_buffers[frame_idx]);
        memcpy(dst, lights.data(), lights.size() * sizeof(Light));
    }

    Rx::Ptr<BindGroup> Renderer::bind_global_resources_for_frame(const Uint32 frame_idx) {
        ZoneScoped;

        auto& material_bind_group_builder = device->get_material_bind_group_builder_for_frame(frame_idx);
        material_bind_group_builder.clear_all_bindings();

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

    Buffer& Renderer::get_model_matrix_for_frame(const Uint32 frame_idx) { return *model_matrix_buffers[frame_idx]; }

    Uint32 Renderer::add_model_matrix_to_frame(const TransformComponent& transform, const Uint32 frame_idx) {
        const auto index = next_unused_model_matrix_per_frame[frame_idx]->fetch_add(1);

        const auto matrix = transform.to_matrix();

        auto* dst = static_cast<glm::mat4*>(model_matrix_buffers[frame_idx]->mapped_ptr);
        memcpy(dst + index, &matrix, sizeof(glm::mat4));

        return index;
    }
} // namespace renderer

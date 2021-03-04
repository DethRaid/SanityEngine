#include "renderer.hpp"

#include "GLFW/glfw3.h"
#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "core/align.hpp"
#include "core/components.hpp"
#include "core/constants.hpp"
#include "entt/entity/registry.hpp"
#include "imgui/imgui.h"
#include "loading/image_loading.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/camera_matrix_buffer.hpp"
#include "renderer/hlsl/shared_structs.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderpasses/fluid_sim_pass.hpp"
#include "renderer/renderpasses/postprocessing_pass.hpp"
#include "renderer/renderpasses/ui_render_pass.hpp"
#include "renderer/rhi/d3d12_private_data.hpp"
#include "renderer/rhi/d3dx12.hpp"
#include "renderer/rhi/helpers.hpp"
#include "renderer/rhi/render_backend.hpp"
#include "rx/console/variable.h"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

namespace sanity::engine::renderer {
    constexpr Uint32 MATERIAL_DATA_BUFFER_SIZE = 1 << 20;

    RX_LOG("Renderer", logger);

    RX_CONSOLE_IVAR(r_max_drawcalls_per_frame,
                    "render.MaxDrawcallsPerFrame",
                    "Maximum number of drawcalls that may be issued in a given frame",
                    1,
                    INT_MAX,
                    100000);

    Renderer::Renderer(GLFWwindow* window)
        : start_time{std::chrono::high_resolution_clock::now()},
          backend{make_render_device(window)},
          camera_matrix_buffers{Rx::make_ptr<CameraMatrixBuffer>(RX_SYSTEM_ALLOCATOR, *this)},
          spd{Rx::make_ptr<SinglePassDownsampler>(RX_SYSTEM_ALLOCATOR, SinglePassDownsampler::Create(*backend))} {
        ZoneScoped;

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        logger->verbose("Setting output framebuffer resolution to %dx%d", width, height);

        output_framebuffer_size = {width, height};

        create_static_mesh_storage();

        allocate_resource_descriptors();

        create_per_frame_buffers();

        create_material_data_buffers();

        create_light_buffers();

        create_builtin_images();

        create_render_passes();

        logger->info("Constructed Renderer");
    }

    void Renderer::reload_shaders() {
        backend->wait_idle();

        reload_builtin_shaders();

        reload_renderpass_shaders();
    }

    void Renderer::begin_frame(const uint64_t frame_count) {
        backend->begin_frame(frame_count);

        const auto cur_time = std::chrono::high_resolution_clock::now();
        const auto duration_since_start = cur_time - start_time;
        const auto ns_since_start = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_start).count();
        const auto time_since_start = static_cast<double>(ns_since_start) / 1000000000.0;

        frame_constants.time_since_start = static_cast<Float32>(time_since_start);
        frame_constants.frame_count = static_cast<Uint32>(frame_count);

        const auto frame_idx = backend->get_cur_gpu_frame_idx();
        next_unused_model_matrix_per_frame[frame_idx]->store(0);
    }

    void Renderer::render_frame(entt::registry& registry) {
        ZoneScoped;

        const auto frame_idx = backend->get_cur_gpu_frame_idx();

        auto command_list = backend->create_command_list(frame_idx);
        set_object_name(command_list, Rx::String::format("Main Render Command List for frame %d", frame_idx));

        {
            TracyD3D12Zone(RenderBackend::tracy_context, *command_list, "Renderer::render_all");
            PIXScopedEvent(*command_list, PIX_COLOR_DEFAULT, "Renderer::render_all");
            if(raytracing_scene_dirty) {
                rebuild_raytracing_scene(command_list);
                raytracing_scene_dirty = false;
            }

            update_cameras(registry, frame_idx);

            upload_material_data(frame_idx);

            update_light_data_buffer(registry, frame_idx);

            update_frame_constants(registry, frame_idx);

            command_list->SetGraphicsRootSignature(*backend->get_standard_root_signature());

            update_resource_array_descriptors(command_list, frame_idx);

            execute_all_render_passes(command_list, registry, frame_idx);
        }

        backend->submit_command_list(Rx::Utility::move(command_list));
    }

    void Renderer::issue_pre_pass_barriers(ID3D12GraphicsCommandList* command_list,
                                           const Uint32 render_pass_index,
                                           const Rx::Ptr<RenderPass>& render_pass) {
        ZoneScoped;

        const auto& used_resources = render_pass->get_texture_states();

        auto barriers = Rx::Vector<D3D12_RESOURCE_BARRIER>{};
        barriers.reserve(used_resources.size());

        const auto& previous_resource_usages = get_previous_resource_states(render_pass_index);

        used_resources.each_pair([&](const TextureHandle& texture_handle, const Rx::Optional<BeginEndState>& before_after_state) {
            if(!before_after_state) {
                return;
            }

            const auto& image = get_texture(texture_handle);
            auto* resource = *image.resource;

            if(const auto* previous_state = previous_resource_usages.find(texture_handle)) {
                if(*previous_state == before_after_state->first) {
                    return;
                }

                // Only issue a barrier if we need a state transition
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, *previous_state, before_after_state->first);

                // Issue the end of a split barrier cause we're the best
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;

                barriers.push_back(barrier);

            } else {
                if(before_after_state->first != D3D12_RESOURCE_STATE_COMMON) {
                    // no previous usage so just barrier from COMMON
                    // No split barrier here, that'd be silly

                    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource,
                                                                               D3D12_RESOURCE_STATE_COMMON,
                                                                               before_after_state->first);

                    barriers.push_back(barrier);
                }
            }
        });

        if(!barriers.is_empty()) {
            command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }
    }

    void Renderer::issue_post_pass_barriers(ID3D12GraphicsCommandList* command_list,
                                            const Uint32 render_pass_index,
                                            const Rx::Ptr<RenderPass>& render_pass) {
        ZoneScoped;

        const auto& used_resources = render_pass->get_texture_states();

        auto barriers = Rx::Vector<D3D12_RESOURCE_BARRIER>{};
        barriers.reserve(used_resources.size());

        const auto& next_resource_usages = get_next_resource_states(render_pass_index);

        used_resources.each_pair([&](const TextureHandle& texture_handle, const Rx::Optional<BeginEndState>& before_after_state) {
            if(!before_after_state) {
                return;
            }

            const auto& image = get_texture(texture_handle);

            if(const auto* next_state = next_resource_usages.find(texture_handle)) {
                if(before_after_state->second == *next_state) {
                    return;
                }

                // Only issue a barrier if we need a state transition
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(image.resource, before_after_state->second, *next_state);

                // Issue the end of a split barrier cause we're the best
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

                barriers.push_back(barrier);

            } else {
                if(before_after_state->second != D3D12_RESOURCE_STATE_COMMON) {
                    // no next usage so just barrier to COMMON
                    // No split barrier here, that'd be silly

                    const auto& barrier = CD3DX12_RESOURCE_BARRIER::Transition(image.resource,
                                                                               before_after_state->second,
                                                                               D3D12_RESOURCE_STATE_COMMON);

                    barriers.push_back(barrier);
                }
            }
        });

        if(!barriers.is_empty()) {
            command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }
    }

    void Renderer::bind_global_resources(ID3D12GraphicsCommandList* command_list) const {
        static_mesh_storage->bind_to_command_list(command_list);
    }

    void Renderer::execute_all_render_passes(engine::ComPtr<ID3D12GraphicsCommandList4>& command_list,
                                             entt::registry& registry,
                                             const Uint32& frame_idx) {
        {
            ZoneScoped;

            TracyD3D12Zone(RenderBackend::tracy_context, *command_list, "Renderer::execute_all_render_passes");
            PIXScopedEvent(*command_list, PIX_COLOR_DEFAULT, "execute_all_render_passes");

            {
                ZoneScopedN("Collect renderpass work");
                render_passes.each_fwd([&](const Rx::Ptr<RenderPass>& pass) { pass->collect_work(registry, frame_idx); });
            }

            {
                ZoneScopedN("Record renderpass work");

                bind_global_resources(command_list);

                for(Uint32 i = 0; i < render_passes.size(); i++) {
                    Rx::Ptr<RenderPass>& render_pass = render_passes[i];

                    issue_pre_pass_barriers(command_list, i, render_pass);

                    render_pass->record_work(command_list, registry, frame_idx);

                    issue_post_pass_barriers(command_list, i, render_pass);
                }
            }
        }
    }

    void Renderer::end_frame() const { backend->end_frame(); }

    void Renderer::add_raytracing_objects_to_scene(const Rx::Vector<RaytracingObject>& new_objects) {
        raytracing_objects.append(new_objects);
        raytracing_scene_dirty = true;
    }

    BufferHandle Renderer::create_buffer(const BufferCreateInfo& create_info) {
        const auto buffer = backend->create_buffer(create_info);
        if(!buffer) {
            return {};
        }

        const auto idx = static_cast<Uint32>(all_buffers.size());
        const auto handle = BufferHandle(idx);

        buffer_name_to_handle.insert(create_info.name, handle);
        all_buffers.push_back(*buffer);

        return handle;
    }

    BufferHandle Renderer::create_buffer(const BufferCreateInfo& create_info, const void* data, ID3D12GraphicsCommandList2* cmds) {
        const auto handle = create_buffer(create_info);
        const auto buffer = get_buffer(handle);

        if(buffer->mapped_ptr != nullptr) {
            memcpy(buffer->mapped_ptr, data, create_info.size);

        } else {
            const auto staging_buffer = backend->get_staging_buffer(create_info.size);
            memcpy(staging_buffer.mapped_ptr, data, create_info.size);

            cmds->CopyBufferRegion(buffer->resource, 0, staging_buffer.resource, 0, create_info.size);
        }

        return handle;
    }

    Rx::Optional<Buffer> Renderer::get_buffer(const Rx::String& name) const {
        if(const auto* handle = buffer_name_to_handle.find(name)) {
            return get_buffer(*handle);
        }

        return Rx::nullopt;
    }

    Rx::Optional<Buffer> Renderer::get_buffer(const BufferHandle& handle) const {
        if(handle.index >= all_buffers.size()) {
            return Rx::nullopt;
        }

        return all_buffers[handle.index];
    }

    TextureHandle Renderer::create_texture(const TextureCreateInfo& create_info) {
        const auto idx = static_cast<Uint32>(all_textures.size());
        const auto handle = TextureHandle(idx);

        auto image = backend->create_texture(create_info);
        if(image) {
            all_textures.push_back(*image);
            texture_name_to_index.insert(create_info.name, handle);

            // logger->verbose("Created texture %s with index %u", create_info.name, idx);

            return handle;

        } else {
            return pink_texture_handle;
        }
    }

    TextureHandle Renderer::create_texture(const TextureCreateInfo& create_info,
                                           const void* image_data,
                                           ID3D12GraphicsCommandList2* commands,
                                           const bool generate_mipmaps) {
        ZoneScoped;

        const auto scope_name = Rx::String::format("create_image(\"%s\")", create_info.name);
        TracyD3D12Zone(RenderBackend::tracy_context, commands, scope_name.data());
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, scope_name.data());

        const auto handle = create_texture(create_info);
        auto& image = all_textures[handle.index];

        if(create_info.usage == TextureUsage::UnorderedAccess) {
            // Transition the image to COPY_DEST
            const auto barriers = Rx::Array{
                CD3DX12_RESOURCE_BARRIER::Transition(image.resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST)};

            commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
        }

        const auto& staging_buffer = backend->get_staging_buffer_for_texture(image.resource);

        const auto pixel_size = size_in_bytes(create_info.format);

        const auto subresource = D3D12_SUBRESOURCE_DATA{
            .pData = image_data,
            .RowPitch = static_cast<LONG_PTR>(create_info.width) * pixel_size,
            .SlicePitch = static_cast<LONG_PTR>(create_info.width) * create_info.height * pixel_size,
        };

        const auto result = UpdateSubresources(commands, image.resource, staging_buffer.resource, 0, 0, 1, &subresource);
        if(result == 0) {
            logger->error("Could not upload texture data");

            return pink_texture_handle;
        }

        if(generate_mipmaps) {
            {
                const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(image.resource,
                                                                                     D3D12_RESOURCE_STATE_COPY_DEST,
                                                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS)};

                commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
            }

            spd->generate_mip_chain_for_texture(image.resource, commands);

            if(create_info.usage == TextureUsage::RenderTarget) {
                const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(image.resource,
                                                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                                     D3D12_RESOURCE_STATE_RENDER_TARGET)};

                commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());

            } else if(create_info.usage == TextureUsage::DepthStencil) {
                const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(image.resource,
                                                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                                     D3D12_RESOURCE_STATE_DEPTH_WRITE)};

                commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());

            } else if(create_info.usage == TextureUsage::SampledTexture) {
                const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(image.resource,
                                                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                                     D3D12_RESOURCE_STATE_GENERIC_READ)};

                commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());

            } else if(create_info.usage == TextureUsage::UnorderedAccess) {
                const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::UAV(image.resource)};
                commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
            }
        }

        return handle;
    }

    Rx::Optional<TextureHandle> Renderer::get_texture_handle(const Rx::String& name) {
        if(const auto* idx = texture_name_to_index.find(name)) { // NOLINT(bugprone-branch-clone)
            return TextureHandle{*idx};

        } else {
            return Rx::nullopt;
        }
    }

    Texture Renderer::get_texture(const Rx::String& name) const {
        if(const auto* idx = texture_name_to_index.find(name)) {
            return all_textures[idx->index];

        } else {
            Rx::abort("Texture '%s' does not exist", name);
        }
    }

    Texture Renderer::get_texture(const TextureHandle handle) const {
        ZoneScoped;

        if(handle == BACKBUFFER_HANDLE) {
            return Texture{};
            //.name = "Backbuffer", .width = output_framebuffer_size.x, .height = output_framebuffer_size.y,      .resource =
            // get_render_backend()->get_back

        } else {
            return all_textures[handle.index];
        }
    }

    void Renderer::schedule_texture_destruction(const TextureHandle& texture_handle) {
        // Intentional copy. We zero out the memory later
        const auto texture = Rx::Utility::move(all_textures[texture_handle.index]);
        backend->schedule_texture_destruction(texture);

        all_textures[texture_handle.index] = {};
    }

    FluidVolumeHandle Renderer::create_fluid_volume(const FluidVolumeCreateInfo& create_info) {

        FluidVolume new_volume{.size = create_info.size, .voxels_per_meter = create_info.voxels_per_meter};
        const auto voxel_size = new_volume.get_voxel_size();

        Rx::Vector<D3D12_RESOURCE_BARRIER> barriers;

        new_volume.density_texture[0] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Density 0", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.density_texture[1] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Density 1", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.temperature_texture[0] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Temperature 0", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.temperature_texture[1] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Temperature 1", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.reaction_texture[0] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Reaction 0", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.reaction_texture[1] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Reaction 1", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.velocity_texture[0] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Velocity 0", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.velocity_texture[1] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Velocity 1", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.pressure_texture[0] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Pressure 0", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.pressure_texture[1] = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Pressure 1", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        new_volume.temp_texture = create_texture(TextureCreateInfo{
            .name = Rx::String::format("%s Temp", create_info.name),
            .usage = TextureUsage::UnorderedAccess,
            .format = TextureFormat::R32F,
            .width = voxel_size.x,
            .height = voxel_size.y,
            .depth = voxel_size.z,
        });

        const auto handle = FluidVolumeHandle(static_cast<Uint32>(all_fluid_volumes.size()));

        all_fluid_volumes.push_back(new_volume);

        return handle;
    }

    FluidVolume& Renderer::get_fluid_volume(const FluidVolumeHandle& handle) { return all_fluid_volumes[handle.index]; }

    void Renderer::set_scene_output_texture(const TextureHandle output_texture_handle) {
        ZoneScoped;

        postprocessing_pass_handle.get()->set_output_texture(output_texture_handle);
    }

    TextureHandle Renderer::get_scene_output_texture() const { return postprocessing_pass_handle.get()->get_output_texture(); }

    StandardMaterialHandle Renderer::allocate_standard_material(const StandardMaterial& material) {
        if(!free_material_handles.is_empty()) {
            const auto& handle = free_material_handles.last();
            free_material_handles.pop_back();
            standard_materials[handle.index] = material;
            return handle;
        }

        const auto handle = StandardMaterialHandle(static_cast<Uint32>(standard_materials.size()));
        standard_materials.push_back(material);

        return handle;
    }

    const BufferHandle& Renderer::get_standard_material_buffer_for_frame(const Uint32 frame_idx) const {
        return material_device_buffers[frame_idx];
    }

    void Renderer::deallocate_standard_material(const StandardMaterialHandle handle) { free_material_handles.push_back(handle); }

    LightHandle Renderer::next_next_free_light_handle() {
        if(available_light_handles.is_empty()) {
            if(next_free_light_index >= MAX_NUM_LIGHTS) {
                Rx::abort("Maximum number of lights already in use! Unable to upload any more");
            }

            const auto handle = LightHandle(next_free_light_index);
            next_free_light_index++;

            return handle;

        } else {
            const auto handle = available_light_handles.last();
            available_light_handles.pop_back();

            return handle;
        }
    }

    void Renderer::return_light_handle(const LightHandle handle) { available_light_handles.push_back(handle); }

    RenderBackend& Renderer::get_render_backend() const { return *backend; }

    MeshDataStore& Renderer::get_static_mesh_store() const { return *static_mesh_storage; }

    void Renderer::begin_device_capture() const { backend->begin_capture(); }

    void Renderer::end_device_capture() const { backend->end_capture(); }

    TextureHandle Renderer::get_noise_texture() const { return noise_texture_handle; }

    TextureHandle Renderer::get_pink_texture() const { return pink_texture_handle; }

    TextureHandle Renderer::get_default_normal_texture() const { return normal_roughness_texture_handle; }

    TextureHandle Renderer::get_default_metallic_roughness_texture() const { return specular_emission_texture_handle; }

    BufferHandle Renderer::get_frame_constants_buffer(const Uint32 frame_idx) const { return frame_constants_buffers[frame_idx]; }

    const RaytracingScene& Renderer::get_raytracing_scene() const { return raytracing_scene; }

    RaytracingAsHandle Renderer::create_raytracing_geometry(const Buffer& vertex_buffer,
                                                            const Buffer& index_buffer,
                                                            const Rx::Vector<PlacedMesh>& meshes,
                                                            ID3D12GraphicsCommandList4* commands) {
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Renderer::create_raytracing_geometry");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Renderer::create_raytracing_geometry");

        Rx::Vector<D3D12_RAYTRACING_GEOMETRY_DESC> geom_descs;
        geom_descs.reserve(meshes.size());
        meshes.each_fwd([&](const PlacedMesh& mesh) {
            const auto transform_buffer = backend->get_staging_buffer(sizeof(glm::mat3x4));
            const auto matrix = glm::mat4x3{glm::vec3{mesh.model_matrix[0]},
                                            glm::vec3{mesh.model_matrix[1]},
                                            glm::vec3{mesh.model_matrix[2]},
                                            glm::vec3{mesh.model_matrix[3]}};
            memcpy(transform_buffer.mapped_ptr, &mesh.model_matrix[0][0], sizeof(glm::mat4x3));

            const auto& [first_vertex, num_vertices, first_index, num_indices] = mesh.mesh;

            // Don't offset the vertex buffer here. We add the vertex offset to the indices when importing the glTF primitive
            auto geom_desc = D3D12_RAYTRACING_GEOMETRY_DESC{.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                                                            .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                                                            .Triangles = {.Transform3x4 = transform_buffer.resource->GetGPUVirtualAddress(),
                                                                          .IndexFormat = DXGI_FORMAT_R32_UINT,
                                                                          .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                                                                          .IndexCount = num_indices,
                                                                          .VertexCount = num_vertices,
                                                                          .IndexBuffer = index_buffer.resource->GetGPUVirtualAddress() +
                                                                                         (first_index * sizeof(Uint32)),
                                                                          .VertexBuffer = {.StartAddress = vertex_buffer.resource
                                                                                                               ->GetGPUVirtualAddress(),
                                                                                           .StrideInBytes = sizeof(StandardVertex)}}};

            geom_descs.push_back(Rx::Utility::move(geom_desc));
            backend->return_staging_buffer(transform_buffer);
        });

        const auto build_as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = static_cast<UINT>(geom_descs.size()),
            .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
            .pGeometryDescs = geom_descs.data()};

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_prebuild_info{};
        backend->device5->GetRaytracingAccelerationStructurePrebuildInfo(&build_as_inputs, &as_prebuild_info);

        as_prebuild_info.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                        as_prebuild_info.ScratchDataSizeInBytes);
        as_prebuild_info.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                          as_prebuild_info.ResultDataMaxSizeInBytes);

        auto scratch_buffer = backend->get_scratch_buffer(static_cast<Uint32>(as_prebuild_info.ScratchDataSizeInBytes));

        const auto result_buffer_create_info = BufferCreateInfo{.name = "BLAS Result Buffer",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<Uint32>(as_prebuild_info.ResultDataMaxSizeInBytes)};

        const auto result_buffer_handle = create_buffer(result_buffer_create_info);
        const auto& result_buffer = get_buffer(result_buffer_handle);

        const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
            .DestAccelerationStructureData = result_buffer->resource->GetGPUVirtualAddress(),
            .Inputs = build_as_inputs,
            .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress()};

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(*result_buffer->resource);
        commands->ResourceBarrier(1, &barrier);

        backend->return_scratch_buffer(Rx::Utility::move(scratch_buffer));

        auto new_ray_geo = RaytracingAccelerationStructure{.blas_buffer = result_buffer_handle};

        const auto handle_idx = static_cast<Uint32>(raytracing_geometries.size());
        raytracing_geometries.push_back(Rx::Utility::move(new_ray_geo));

        return RaytracingAsHandle(handle_idx);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE Renderer::get_resource_array_gpu_descriptor(const Uint32 frame_idx) const {
        return resource_descriptors[frame_idx].gpu_handle;
    }

    void Renderer::create_static_mesh_storage() {
        const auto vertex_create_info = BufferCreateInfo{
            .name = "Static Mesh Vertex Buffer",
            .usage = BufferUsage::VertexBuffer,
            .size = STATIC_MESH_VERTEX_BUFFER_SIZE,
        };

        auto vertex_buffer = create_buffer(vertex_create_info);

        const auto index_buffer_create_info = BufferCreateInfo{.name = "Static Mesh Index Buffer",
                                                               .usage = BufferUsage::IndexBuffer,
                                                               .size = STATIC_MESH_INDEX_BUFFER_SIZE};

        auto index_buffer = create_buffer(index_buffer_create_info);

        static_mesh_storage = Rx::make_ptr<MeshDataStore>(RX_SYSTEM_ALLOCATOR, *this, vertex_buffer, index_buffer);
    }

    void Renderer::allocate_resource_descriptors() {
        ZoneScoped;

        auto& descriptors = backend->get_cbv_srv_uav_allocator();

        const auto num_gpu_frames = backend->get_max_num_gpu_frames();

        resource_descriptors.resize(num_gpu_frames);

        for(auto i = 0u; i < num_gpu_frames; i++) {
            resource_descriptors[i] = descriptors.allocate_descriptors(MAX_NUM_BUFFERS + MAX_NUM_TEXTURES);
        }
    }

    void Renderer::create_per_frame_buffers() {
        ZoneScoped;

        const auto num_gpu_frames = backend->get_max_num_gpu_frames();

        frame_constants_buffers.reserve(num_gpu_frames);
        model_matrix_buffers.reserve(num_gpu_frames);

        auto frame_constants_buffer_create_info = BufferCreateInfo{
            .usage = BufferUsage::ConstantBuffer,
            .size = sizeof(FrameConstants),
        };

        auto model_matrix_buffer_create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer,
                                                                .size = static_cast<Uint32>(sizeof(glm::mat4) *
                                                                                            r_max_drawcalls_per_frame->get())};

        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            frame_constants_buffer_create_info.name = Rx::String::format("Frame constants buffer %d", i);
            const auto frame_constants_buffer = create_buffer(frame_constants_buffer_create_info);
            if(frame_constants_buffer.is_valid()) {
                frame_constants_buffers.push_back(frame_constants_buffer);

            } else {
                logger->error("Could not create buffer %s", frame_constants_buffer_create_info.name);
            }

            model_matrix_buffer_create_info.name = Rx::String::format("Model matrix buffer %d", i);
            const auto model_matrix_buffer = create_buffer(model_matrix_buffer_create_info);
            if(model_matrix_buffer.is_valid()) {
                model_matrix_buffers.push_back(model_matrix_buffer);

            } else {
                logger->error("Could not create buffer %s", model_matrix_buffer_create_info.name);
            }

            next_unused_model_matrix_per_frame.emplace_back(Rx::make_ptr<Rx::Concurrency::Atomic<Uint32>>(RX_SYSTEM_ALLOCATOR, 0_u32));
        }
    }

    void Renderer::create_material_data_buffers() {
        ZoneScoped;

        const auto num_gpu_frames = backend->get_max_num_gpu_frames();

        auto create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer, .size = MATERIAL_DATA_BUFFER_SIZE};
        material_device_buffers.reserve(num_gpu_frames);
        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            create_info.name = Rx::String::format("Material Data Buffer %d", i);
            const auto material_buffer = create_buffer(create_info);
            if(material_buffer.is_valid()) {
                material_device_buffers.push_back(material_buffer);

            } else {
                logger->error("Could not create buffer %s", create_info.name);
            }
        }
    }

    void Renderer::create_light_buffers() {
        ZoneScoped;

        lights.resize(MAX_NUM_LIGHTS);

        const auto num_gpu_frames = backend->get_max_num_gpu_frames();

        auto create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer, .size = MAX_NUM_LIGHTS * sizeof(GpuLight)};

        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            create_info.name = Rx::String::format("Light Buffer %d", i);
            const auto light_buffer = create_buffer(create_info);
            if(light_buffer.is_valid()) {
                light_device_buffers.push_back(light_buffer);

            } else {
                logger->error("Could not create buffer %s", create_info.name);
            }
        }
    }

    void Renderer::create_builtin_images() {
        ZoneScoped;

        load_noise_texture("data/textures/noise/LDR_RGBA_0.png");
        // load_noise_texture("data/textures/noise/RuthNoise.png");

        auto commands = backend->create_command_list();
        set_object_name(*commands, "Renderer::create_builtin_images");

        {
            TracyD3D12Zone(RenderBackend::tracy_context, commands, "Renderer::create_builtin_images");
            PIXScopedEvent(*commands, PIX_COLOR_DEFAULT, "Renderer::create_builtin_images");

            {
                const auto pink_texture_create_info = TextureCreateInfo{.name = "Pink",
                                                                        .usage = TextureUsage::SampledTexture,
                                                                        .format = TextureFormat::Rgba8,
                                                                        .width = 8,
                                                                        .height = 8};

                auto pink_texture_pixel = Rx::Vector<Uint32>{};
                pink_texture_pixel.reserve(64);
                for(Uint32 i = 0; i < 64; i++) {
                    pink_texture_pixel.push_back(0xFFFF00FF);
                }

                pink_texture_handle = create_texture(pink_texture_create_info, pink_texture_pixel.data(), commands);
            }

            {
                const auto normal_roughness_texture_create_info = TextureCreateInfo{.name = "Default Normal",
                                                                                    .usage = TextureUsage::SampledTexture,
                                                                                    .format = TextureFormat::Rgba8,
                                                                                    .width = 8,
                                                                                    .height = 8};

                auto normal_roughness_texture_pixel = Rx::Vector<Uint32>{};
                normal_roughness_texture_pixel.reserve(64);
                for(Uint32 i = 0; i < 64; i++) {
                    normal_roughness_texture_pixel.push_back(0x80FF8080);
                }

                normal_roughness_texture_handle = create_texture(normal_roughness_texture_create_info,
                                                                 normal_roughness_texture_pixel.data(),
                                                                 commands);
            }

            {
                const auto specular_emission_texture_create_info = TextureCreateInfo{.name = "Default Metallic/Roughness",
                                                                                     .usage = TextureUsage::SampledTexture,
                                                                                     .format = TextureFormat::Rgba8,
                                                                                     .width = 8,
                                                                                     .height = 8};

                auto specular_emission_texture_pixel = Rx::Vector<Uint32>{};
                specular_emission_texture_pixel.reserve(64);
                for(Uint32 i = 0; i < 64; i++) {
                    specular_emission_texture_pixel.push_back(0x00A00000);
                }

                specular_emission_texture_handle = create_texture(specular_emission_texture_create_info,
                                                                  specular_emission_texture_pixel.data(),
                                                                  commands);
            }
        }

        backend->submit_command_list(Rx::Utility::move(commands));
    }

    void Renderer::load_noise_texture(const std::filesystem::path& filepath) {
        ZoneScoped;

        const auto handle = load_texture_to_gpu(filepath, *this);

        if(!handle) {
            logger->error("Could not load noise texture %s", filepath);
            return;
        }

        noise_texture_handle = *handle;
    }

    void Renderer::create_render_passes() {
        render_passes.reserve(5);

        render_passes.push_back(Rx::make_ptr<FluidSimPass>(RX_SYSTEM_ALLOCATOR, *this));
        fluid_sim_pass_handle = RenderpassHandle<FluidSimPass>::make_from_last_element(render_passes);

        render_passes.push_back(Rx::make_ptr<DirectLightingPass>(RX_SYSTEM_ALLOCATOR, *this, output_framebuffer_size));
        direct_lighting_pass_handle = RenderpassHandle<DirectLightingPass>::make_from_last_element(render_passes);

        render_passes.push_back(Rx::make_ptr<DenoiserPass>(RX_SYSTEM_ALLOCATOR,
                                                           *this,
                                                           output_framebuffer_size,
                                                           static_cast<DirectLightingPass&>(*render_passes[1])));
        denoiser_pass_handle = RenderpassHandle<DenoiserPass>::make_from_last_element(render_passes);

        render_passes.push_back(
            Rx::make_ptr<PostprocessingPass>(RX_SYSTEM_ALLOCATOR, *this, static_cast<DenoiserPass&>(*render_passes[2])));
        postprocessing_pass_handle = RenderpassHandle<PostprocessingPass>::make_from_last_element(render_passes);

        render_passes.push_back(Rx::make_ptr<DearImGuiRenderPass>(RX_SYSTEM_ALLOCATOR, *this));
        imgui_pass_handle = RenderpassHandle<DearImGuiRenderPass>::make_from_last_element(render_passes);
    }

    void Renderer::reload_builtin_shaders() { throw std::runtime_error{"Not implemented"}; }

    void Renderer::reload_renderpass_shaders() { throw std::runtime_error{"Not implemented"}; }

    const Rx::Vector<Texture>& Renderer::get_texture_array() const { return all_textures; }

    void Renderer::update_cameras(entt::registry& registry, const Uint32 frame_idx) const {
        ZoneScoped;

        registry.view<TransformComponent, CameraComponent>().each([&](const TransformComponent& transform, const CameraComponent& camera) {
            // logger->verbose(
            //     "[%d] Updating matrices for camera %d\nTransform:\n\tLocation = (%f, %f, %f)\n\tRotation = (%f, %f, %f, %f)\n\tScale =
            //     (%f, %f, %f)", frame_idx, camera.idx, transform.location.x, transform.location.y, transform.location.z,
            //     transform.rotation.x,
            //     transform.rotation.y,
            //     transform.rotation.z,
            //     transform.rotation.w,
            //     transform.scale.x,
            //     transform.scale.y,
            //     transform.scale.z);

            auto& matrices = camera_matrix_buffers->get_camera_matrices(camera.idx);

            matrices.copy_matrices_to_previous();
            matrices.calculate_view_matrix(transform);
            matrices.calculate_projection_matrix(camera);
        });

        camera_matrix_buffers->upload_data(frame_idx);
    }

    void Renderer::upload_material_data(const Uint32 frame_idx) {
        ZoneScoped;

        const auto& buffer_handle = material_device_buffers[frame_idx];
        const auto& buffer = get_buffer(buffer_handle);
        memcpy(buffer->mapped_ptr, standard_materials.data(), standard_materials.size() * sizeof(StandardMaterial));
    }

    void Renderer::update_resource_array_descriptors(ID3D12GraphicsCommandList* cmds, const Uint32 frame_idx) {
        ZoneScoped;
        const auto& resource_descriptors_range = resource_descriptors[frame_idx];

        // Intentional copy
        auto write_descriptor = resource_descriptors_range.cpu_handle;

        auto* device = backend->get_d3d12_device();
        const auto descriptor_size = backend->get_cbv_srv_uav_allocator().get_descriptor_size();

        for(auto i = 0u; i < all_buffers.size(); i++) {
            const auto& buffer = all_buffers[i];
            if(!buffer.resource) {
                continue;
            }

            // V0: bind all buffers as SRVs. The debug layers should yell at us if this is bad
            const auto desc = D3D12_SHADER_RESOURCE_VIEW_DESC{.Format = DXGI_FORMAT_R32_TYPELESS,
                                                              .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                                                              .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                                              .Buffer = D3D12_BUFFER_SRV{.FirstElement = 0,
                                                                                         .NumElements = static_cast<UINT>(buffer.size /
                                                                                                                          4.0),
                                                                                         .StructureByteStride = 0,
                                                                                         .Flags = D3D12_BUFFER_SRV_FLAG_RAW}};

            device->CreateShaderResourceView(buffer.resource, &desc, write_descriptor);

            write_descriptor.Offset(1, descriptor_size);
        }

        write_descriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE{resource_descriptors_range.cpu_handle, MAX_NUM_BUFFERS, descriptor_size};
        for(int i = 0u; i < all_textures.size(); i++) {
            const auto& texture = all_textures[i];
            if(!texture.resource) {
                continue;
            }

            const auto& texture_desc = texture.resource->GetDesc();

            auto format = to_dxgi_format(texture.format);
            Uint32 mapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if(format == DXGI_FORMAT_D32_FLOAT) {
                format = DXGI_FORMAT_R32_FLOAT;
                mapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 0, 0, 0);
            }

            // V0: Bind all the textures as SRVs
            D3D12_SHADER_RESOURCE_VIEW_DESC desc;
            if(texture.depth == 1) {
                desc = D3D12_SHADER_RESOURCE_VIEW_DESC{.Format = format,
                                                       .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                                                       .Shader4ComponentMapping = mapping,
                                                       .Texture2D = D3D12_TEX2D_SRV{.MostDetailedMip = 0,
                                                                                    .MipLevels = texture_desc.MipLevels,
                                                                                    .PlaneSlice = 0,
                                                                                    .ResourceMinLODClamp = 0}};
            } else {
                desc = D3D12_SHADER_RESOURCE_VIEW_DESC{.Format = format,
                                                       .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D,
                                                       .Shader4ComponentMapping = mapping,
                                                       .Texture3D = D3D12_TEX3D_SRV{.MostDetailedMip = 0,
                                                                                    .MipLevels = texture_desc.MipLevels,
                                                                                    .ResourceMinLODClamp = 0}};
            }
            device->CreateShaderResourceView(texture.resource, &desc, write_descriptor);

            write_descriptor.Offset(1, descriptor_size);
        }
    }

    Rx::Map<TextureHandle, D3D12_RESOURCE_STATES> Renderer::get_previous_resource_states(const Uint32 cur_renderpass_index) const {
        const auto& used_resources = render_passes[cur_renderpass_index]->get_texture_states();
        auto previous_states = Rx::Map<TextureHandle, D3D12_RESOURCE_STATES>{};

        if(cur_renderpass_index > 0) {
            for(Int32 i = cur_renderpass_index - 1; i >= 0; i--) {
                used_resources.each_key([&](const TextureHandle& texture) {
                    const auto& renderpass_resources = render_passes[i]->get_texture_states();
                    if(previous_states.find(texture) == nullptr) {
                        // No usage for this resource has been found yes
                        if(const auto* states = renderpass_resources.find(texture); states != nullptr && states->has_value()) {
                            // This renderpass has an entry for the resource we care about!
                            previous_states.insert(texture, (*states)->second);
                        }
                    }
                });
            }
        }

        return previous_states;
    }

    Rx::Map<TextureHandle, D3D12_RESOURCE_STATES> Renderer::get_next_resource_states(const Uint32 cur_renderpass_index) const {
        const auto& used_resources = render_passes[cur_renderpass_index]->get_texture_states();
        auto next_states = Rx::Map<TextureHandle, D3D12_RESOURCE_STATES>{};

        if(cur_renderpass_index > 0) {
            for(Int32 i = cur_renderpass_index + 1; i < render_passes.size(); i++) {
                used_resources.each_key([&](const TextureHandle& texture) {
                    const auto& renderpass_resources = render_passes[i]->get_texture_states();
                    if(next_states.find(texture) == nullptr) {
                        // No usage for this resource has been found yes
                        if(const auto* states = renderpass_resources.find(texture); states != nullptr && states->has_value()) {
                            // This renderpass has an entry for the resource we care about!
                            next_states.insert(texture, (*states)->first);
                        }
                    }
                });
            }
        }

        return next_states;
    }

    void Renderer::rebuild_raytracing_scene(const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        TracyD3D12Zone(RenderBackend::tracy_context, *commands, "RebuildRaytracingScene");
        PIXScopedEvent(*commands, PIX_COLOR_DEFAULT, "Renderer::rebuild_raytracing_scene");

        // TODO: figure out how to update the raytracing scene without needing a full rebuild

        if(has_raytracing_scene) {
            const auto rt_scene_buffer = get_buffer(raytracing_scene.buffer);
            backend->schedule_buffer_destruction(*rt_scene_buffer);
        }

        if(!raytracing_objects.is_empty()) {
            logger->verbose("Building top-level acceleration structure for %u objects", raytracing_objects.size());
            constexpr auto max_num_objects = UINT32_MAX / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

            RX_ASSERT(raytracing_objects.size() < max_num_objects, "May not have more than %u objects because uint32", max_num_objects);

            const auto instance_buffer_size = static_cast<Uint32>(raytracing_objects.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
            auto instance_buffer = backend->get_staging_buffer(instance_buffer_size);
            set_object_name(instance_buffer.resource, "Raytracing instance description buffer");
            auto* instance_buffer_array = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instance_buffer.mapped_ptr);

            for(Uint32 i = 0; i < raytracing_objects.size(); i++) {
                const auto& object = raytracing_objects[i];
                auto& desc = instance_buffer_array[i];
                desc = {};

                const auto model_matrix = object.transform;

                desc.Transform[0][0] = model_matrix[0][0];
                desc.Transform[0][1] = model_matrix[0][1];
                desc.Transform[0][2] = model_matrix[0][2];
                desc.Transform[0][3] = model_matrix[0][3];

                desc.Transform[1][0] = model_matrix[1][0];
                desc.Transform[1][1] = model_matrix[1][1];
                desc.Transform[1][2] = model_matrix[1][2];
                desc.Transform[1][3] = model_matrix[1][3];

                desc.Transform[2][0] = model_matrix[2][0];
                desc.Transform[2][1] = model_matrix[2][1];
                desc.Transform[2][2] = model_matrix[2][2];
                desc.Transform[2][3] = model_matrix[2][3];

                // TODO: Figure out if we want to use the mask to control which kind of rays can hit which objects
                desc.InstanceMask = 0xFF;

                desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

                desc.InstanceContributionToHitGroupIndex = object.material.handle;

                const auto& ray_geo = raytracing_geometries[object.as_handle.index];

                const auto& buffer = get_buffer(ray_geo.blas_buffer);
                desc.AccelerationStructure = buffer->resource->GetGPUVirtualAddress();
            }

            const auto as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
                .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
                .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
                .NumDescs = static_cast<UINT>(raytracing_objects.size()),
                .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
                .InstanceDescs = instance_buffer.resource->GetGPUVirtualAddress(),
            };

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild_info{};
            backend->device5->GetRaytracingAccelerationStructurePrebuildInfo(&as_inputs, &prebuild_info);

            prebuild_info.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                         prebuild_info.ScratchDataSizeInBytes);
            prebuild_info.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                           prebuild_info.ResultDataMaxSizeInBytes);

            auto scratch_buffer = backend->get_scratch_buffer(static_cast<Uint32>(prebuild_info.ScratchDataSizeInBytes));

            const auto as_buffer_create_info = BufferCreateInfo{.name = "Raytracing Scene",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<Uint32>(prebuild_info.ResultDataMaxSizeInBytes)};
            const auto as_buffer_handle = create_buffer(as_buffer_create_info);
            auto as_buffer = get_buffer(as_buffer_handle);

            const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
                .DestAccelerationStructureData = as_buffer->resource->GetGPUVirtualAddress(),
                .Inputs = as_inputs,
                .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress(),
            };

            commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

            const Rx::Vector<D3D12_RESOURCE_BARRIER> barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::UAV(*as_buffer->resource)};
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

            raytracing_scene = {.buffer = as_buffer_handle};
            has_raytracing_scene = true;

            backend->return_staging_buffer(Rx::Utility::move(instance_buffer));
            backend->return_scratch_buffer(Rx::Utility::move(scratch_buffer));
        }
    }

    void Renderer::update_light_data_buffer(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        const auto& view = registry.view<LightComponent, TransformComponent>();
        view.each([&](const LightComponent& light_component, const TransformComponent& transform) {
            auto& light = lights[light_component.handle.index];

            light.type = light_component.type;
            light.color = light_component.color;
            light.size = light_component.size;

            if(light_component.type == LightType::Directional) {
                light.direction_or_location = transform->get_forward_vector();

            } else {
                light.direction_or_location = transform->location;
            }
        });

        const auto& light_buffer_handle = light_device_buffers[frame_idx];
        const auto& light_buffer = get_buffer(light_buffer_handle);
        auto* dst = backend->map_buffer(*light_buffer);
        memcpy(dst, lights.data(), lights.size() * sizeof(GpuLight));
    }

    void Renderer::update_frame_constants(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        frame_constants.render_size = output_framebuffer_size;
        frame_constants.time_since_start = 0;
        frame_constants.frame_count = g_engine->get_frame_count();

        frame_constants.camera_buffer_index = camera_matrix_buffers->get_device_buffer_for_frame(frame_idx).index;
        frame_constants.light_buffer_index = light_device_buffers[frame_idx].index;
        frame_constants.vertex_data_buffer_index = static_mesh_storage->get_vertex_buffer_handle().index;
        frame_constants.index_buffer_index = static_mesh_storage->get_index_buffer_handle().index;

        frame_constants.noise_texture_idx = noise_texture_handle.index;

        frame_constants.sky_texture_idx = 0;
        const auto view = registry.view<SkyComponent>();
        if(view.size() == 1) {
            view.each([&](const SkyComponent& skybox) {
                if(skybox.skybox_texture.is_valid()) {
                    frame_constants.sky_texture_idx = skybox.skybox_texture.index;
                }
            });
        }

        const auto buffer = get_buffer(frame_constants_buffers[frame_idx]);

        memcpy(buffer->mapped_ptr, &frame_constants, sizeof(FrameConstants));
    }

    BufferHandle& Renderer::get_model_matrix_for_frame(const Uint32 frame_idx) { return model_matrix_buffers[frame_idx]; }

    Uint32 Renderer::add_model_matrix_to_frame(const glm::mat4& model_matrix, const Uint32 frame_idx) {
        const auto index = next_unused_model_matrix_per_frame[frame_idx]->fetch_add(1);

        const auto& model_matrix_buffer = get_buffer(model_matrix_buffers[frame_idx]);
        auto* dst = static_cast<glm::mat4*>(model_matrix_buffer->mapped_ptr);
        memcpy(dst + index, &model_matrix, sizeof(glm::mat4));

        return index;
    }

    SinglePassDownsampler& Renderer::get_spd() const { return *spd; }
} // namespace sanity::engine::renderer

#include "compute_command_list.hpp"

#include <minitrace.h>
#include <spdlog/spdlog.h>

#include "../core/align.hpp"
#include "../core/defer.hpp"
#include "../core/ensure.hpp"
#include "bind_group.hpp"
#include "compute_pipeline_state.hpp"
#include "d3dx12.hpp"
#include "render_device.hpp"

namespace rhi {
    ComputeCommandList::ComputeCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in)
        : ResourceCommandList{std::move(cmds), device_in} {}

    ComputeCommandList::ComputeCommandList(ComputeCommandList&& old) noexcept
        : ResourceCommandList(std::move(old)),
          compute_pipeline{old.compute_pipeline},
          current_mesh_data{old.current_mesh_data},
          are_compute_resources_bound{old.are_compute_resources_bound} {}

    ComputeCommandList& ComputeCommandList::operator=(ComputeCommandList&& old) noexcept {
        compute_pipeline = old.compute_pipeline;
        are_compute_resources_bound = old.are_compute_resources_bound;
        current_mesh_data = old.current_mesh_data;

        return static_cast<ComputeCommandList&>(ResourceCommandList::operator=(std::move(old)));
    }

    void ComputeCommandList::set_pipeline_state(const ComputePipelineState& state) {
        if(compute_pipeline == nullptr) {
            // First time binding a compute pipeline to this command list, we need to bind the root signature
            commands->SetComputeRootSignature(state.root_signature.Get());
            are_compute_resources_bound = false;

        } else if(state.root_signature != compute_pipeline->root_signature) {
            // Previous compute pipeline had a different root signature. Bind the new root signature
            commands->SetComputeRootSignature(state.root_signature.Get());
            are_compute_resources_bound = false;
        }

        compute_pipeline = &state;

        commands->SetPipelineState(state.pso.Get());

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    void ComputeCommandList::bind_compute_resources(const BindGroup& bind_group) {
        ENSURE(compute_pipeline != nullptr, "Can not bind compute resources to a command list before you bind a compute pipeline");

        for(const auto& image : bind_group.used_images) {
            set_resource_state(*image.resource, image.states);
        }

        for(const auto& buffer : bind_group.used_buffers) {
            set_resource_state(*buffer.resource, buffer.states);
        }

        if(current_descriptor_heap != bind_group.heap) {
            commands->SetDescriptorHeaps(1, &bind_group.heap);
            current_descriptor_heap = bind_group.heap;
        }

        bind_group.bind_to_compute_signature(*commands.Get());

        are_compute_resources_bound = true;

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    void ComputeCommandList::dispatch(const uint32_t workgroup_x, const uint32_t workgroup_y, const uint32_t workgroup_z) {
#ifndef NDEBUG
        ENSURE(compute_pipeline != nullptr, "Can not dispatch a compute workgroup before binding a compute pipeline");

        if(workgroup_x == 0) {
            spdlog::warn("Your workgroup has a width of 0. Are you sure you want to do that?");
        }

        if(workgroup_y == 0) {
            spdlog::warn("Your workgroup has a height of 0. Are you sure you want to do that?");
        }

        if(workgroup_z == 0) {
            spdlog::warn("Your workgroup has a depth of 0. Are you sure you want to do that?");
        }

        if(!are_compute_resources_bound) {
            // TODO: Disable this warning if it proves useless
            // TODO: Promote to an error if it proves useful
            spdlog::warn("Dispatching a compute job with no resource bound! Are you sure?");
        }
#endif

        if(compute_pipeline != nullptr) {
            commands->Dispatch(workgroup_x, workgroup_y, workgroup_z);
        }

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    void ComputeCommandList::bind_mesh_data(const MeshDataStore& mesh_data) {
        const auto& vertex_bindings = mesh_data.get_vertex_bindings();

        // If we have more than 16 vertex attributes, we probably have bigger problems
        std::array<D3D12_VERTEX_BUFFER_VIEW, 16> vertex_buffer_views{};
        for(uint32_t i = 0; i < vertex_bindings.size(); i++) {
            const auto& binding = vertex_bindings[i];
            const auto* buffer = static_cast<const Buffer*>(binding.buffer);

            set_resource_state(*buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            D3D12_VERTEX_BUFFER_VIEW view{};
            view.BufferLocation = buffer->resource->GetGPUVirtualAddress() + binding.offset;
            view.SizeInBytes = buffer->size - binding.offset;
            view.StrideInBytes = binding.vertex_size;

            vertex_buffer_views[i] = view;
        }

        commands->IASetVertexBuffers(0, static_cast<UINT>(vertex_bindings.size()), vertex_buffer_views.data());

        const auto& index_buffer = mesh_data.get_index_buffer();

        set_resource_state(index_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        D3D12_INDEX_BUFFER_VIEW index_view{};
        index_view.BufferLocation = index_buffer.resource->GetGPUVirtualAddress();
        index_view.SizeInBytes = index_buffer.size;
        index_view.Format = DXGI_FORMAT_R32_UINT;

        commands->IASetIndexBuffer(&index_view);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        current_mesh_data = &mesh_data;
        is_mesh_data_bound = true;
    }

    RaytracingMesh ComputeCommandList::build_acceleration_structure_for_mesh(const uint32_t num_vertices,
                                                                             const uint32_t num_indices,
                                                                             const uint32_t first_index) {
        ENSURE(current_mesh_data != nullptr, "Must have mesh data bound before building acceleration structures out of it");

        const auto& index_buffer = current_mesh_data->get_index_buffer();

        const auto& vertex_buffer = *current_mesh_data->get_vertex_bindings()[0].buffer;

        const auto geom_desc = D3D12_RAYTRACING_GEOMETRY_DESC{.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                                                              .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                                                              .Triangles =
                                                                  {.Transform3x4 = 0,
                                                                   .IndexFormat = DXGI_FORMAT_R32_UINT,
                                                                   .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                                                                   .IndexCount = num_indices,
                                                                   .VertexCount = num_vertices,
                                                                   .IndexBuffer = index_buffer.resource->GetGPUVirtualAddress() +
                                                                                  (first_index * sizeof(uint32_t)),
                                                                   .VertexBuffer = {.StartAddress = vertex_buffer.resource
                                                                                                        ->GetGPUVirtualAddress(),
                                                                                    .StrideInBytes = sizeof(StandardVertex)}}};

        const auto build_as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = 1,
            .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
            .pGeometryDescs = &geom_desc};

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_prebuild_info{};
        device->device5->GetRaytracingAccelerationStructurePrebuildInfo(&build_as_inputs, &as_prebuild_info);

        as_prebuild_info.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                        as_prebuild_info.ScratchDataSizeInBytes);
        as_prebuild_info.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                          as_prebuild_info.ResultDataMaxSizeInBytes);

        auto scratch_buffer = device->get_scratch_buffer(static_cast<uint32_t>(as_prebuild_info.ScratchDataSizeInBytes));

        const auto result_buffer_create_info = BufferCreateInfo{.name = "BLAS Result Buffer",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<uint32_t>(as_prebuild_info.ResultDataMaxSizeInBytes)};

        auto result_buffer = device->create_buffer(result_buffer_create_info);

        const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
            .DestAccelerationStructureData = result_buffer->resource->GetGPUVirtualAddress(),
            .Inputs = build_as_inputs,
            .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress()};

        DEFER(a, [&]() { device->return_scratch_buffer(std::move(scratch_buffer)); });

        set_resource_state(index_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        set_resource_state(vertex_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(result_buffer->resource.Get());
        commands->ResourceBarrier(1, &barrier);

        set_resource_state(*result_buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        return {std::move(result_buffer)};
    }

    RaytracingMesh ComputeCommandList::build_acceleration_structure_for_meshes(const std::vector<Mesh>& meshes) {
        ENSURE(current_mesh_data != nullptr, "Must have mesh data bound before building acceleration structures out of it");

        const auto& index_buffer = current_mesh_data->get_index_buffer();

        const auto& vertex_buffer = *current_mesh_data->get_vertex_bindings()[0].buffer;

        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geom_descs;
        geom_descs.reserve(meshes.size());
        for(const auto& [first_vertex, num_vertices, first_index, num_indices] : meshes) {
            auto geom_desc = D3D12_RAYTRACING_GEOMETRY_DESC{.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                                                            .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                                                            .Triangles =
                                                                {.Transform3x4 = 0,
                                                                 .IndexFormat = DXGI_FORMAT_R32_UINT,
                                                                 .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                                                                 .IndexCount = num_indices,
                                                                 .VertexCount = num_vertices,
                                                                 .IndexBuffer = index_buffer.resource->GetGPUVirtualAddress() +
                                                                                (first_index * sizeof(uint32_t)),
                                                                 .VertexBuffer =
                                                                     {.StartAddress = vertex_buffer.resource->GetGPUVirtualAddress(),
                                                                      .StrideInBytes = sizeof(StandardVertex)}}};

            geom_descs.push_back(std::move(geom_desc));
        }

        const auto build_as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = static_cast<UINT>(geom_descs.size()),
            .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
            .pGeometryDescs = geom_descs.data()};

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_prebuild_info{};
        device->device5->GetRaytracingAccelerationStructurePrebuildInfo(&build_as_inputs, &as_prebuild_info);

        as_prebuild_info.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                        as_prebuild_info.ScratchDataSizeInBytes);
        as_prebuild_info.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                          as_prebuild_info.ResultDataMaxSizeInBytes);

        auto scratch_buffer = device->get_scratch_buffer(static_cast<uint32_t>(as_prebuild_info.ScratchDataSizeInBytes));

        const auto result_buffer_create_info = BufferCreateInfo{.name = "BLAS Result Buffer",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<uint32_t>(as_prebuild_info.ResultDataMaxSizeInBytes)};

        auto result_buffer = device->create_buffer(result_buffer_create_info);

        const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
            .DestAccelerationStructureData = result_buffer->resource->GetGPUVirtualAddress(),
            .Inputs = build_as_inputs,
            .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress()};

        DEFER(a, [&]() { device->return_scratch_buffer(std::move(scratch_buffer)); });

        set_resource_state(index_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        set_resource_state(vertex_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(result_buffer->resource.Get());
        commands->ResourceBarrier(1, &barrier);

        set_resource_state(*result_buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        return {std::move(result_buffer)};
    }

    RaytracingScene ComputeCommandList::build_raytracing_scene(const std::vector<RaytracingObject>& objects) {
        constexpr auto max_num_objects = UINT32_MAX / sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

        ENSURE(objects.size() < max_num_objects, "May not have more than {} objects because uint32", max_num_objects);

        const auto instance_buffer_size = static_cast<uint32_t>(objects.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        auto instance_buffer = device->get_staging_buffer(instance_buffer_size);
        auto* instance_buffer_array = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instance_buffer.ptr);

        for(uint32_t i = 0; i < objects.size(); i++) {
            const auto& object = objects[i];
            auto& desc = instance_buffer_array[i];
            desc = {};

            // TODO: Actually copy the matrix once we get real model matrices
            desc.Transform[0][0] = desc.Transform[1][1] = desc.Transform[2][2] = 1;

            // TODO: Figure out if we want to use the mask to control which kind of rays can hit which objects
            desc.InstanceMask = 0xFF;

            desc.InstanceContributionToHitGroupIndex = object.material.handle;

            const auto& buffer = static_cast<const Buffer&>(*object.blas_buffer);
            desc.AccelerationStructure = buffer.resource->GetGPUVirtualAddress();
        }

        const auto as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = static_cast<UINT>(objects.size()),
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

        const auto as_buffer_create_info = BufferCreateInfo{.name = "Raytracing Scene",
                                                            .usage = BufferUsage::RaytracingAccelerationStructure,
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

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(as_buffer->resource.Get());
        commands->ResourceBarrier(1, &barrier);

        set_resource_state(*as_buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        return {.buffer = std::move(as_buffer)};
    }
} // namespace rhi


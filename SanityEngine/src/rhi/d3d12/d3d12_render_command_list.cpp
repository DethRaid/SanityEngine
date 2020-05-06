#include "d3d12_render_command_list.hpp"

#include <array>
#include <cassert>

#include <minitrace.h>

#include "../../core/align.hpp"
#include "../../core/defer.hpp"
#include "../../core/ensure.hpp"
#include "../mesh_data_store.hpp"
#include "d3d12_bind_group.hpp"
#include "d3d12_framebuffer.hpp"
#include "d3d12_render_pipeline_state.hpp"
#include "d3dx12.hpp"
#include "helpers.hpp"

using std::move;

namespace rhi {
    D3D12RenderCommandList::D3D12RenderCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, D3D12RenderDevice& device_in)
        : D3D12ComputeCommandList{move(cmds), device_in} {}

    D3D12RenderCommandList::D3D12RenderCommandList(D3D12RenderCommandList&& old) noexcept
        : D3D12ComputeCommandList{move(old)},
          in_render_pass{old.in_render_pass},
          current_render_pipeline_state{old.current_render_pipeline_state},
          is_render_material_bound{old.is_render_material_bound},
          is_mesh_data_bound{old.is_mesh_data_bound} {}

    D3D12RenderCommandList& D3D12RenderCommandList::operator=(D3D12RenderCommandList&& old) noexcept {
        in_render_pass = old.in_render_pass;
        current_render_pipeline_state = old.current_render_pipeline_state;
        is_render_material_bound = old.is_render_material_bound;
        is_mesh_data_bound = old.is_mesh_data_bound;

        return static_cast<D3D12RenderCommandList&>(D3D12ComputeCommandList::operator=(move(old)));
    }

    void D3D12RenderCommandList::set_framebuffer(const Framebuffer& framebuffer,
                                                 std::vector<RenderTargetAccess> render_target_accesses,
                                                 std::optional<RenderTargetAccess> depth_access) {
        MTR_SCOPE("D3D12RenderCommandList", "set_render_targets");

        const D3D12Framebuffer& d3d12_framebuffer = static_cast<const D3D12Framebuffer&>(framebuffer);

        ENSURE(d3d12_framebuffer.rtv_handles.size() == render_target_accesses.size(),
               "Must have the same number of render targets and render target accesses");
        ENSURE(d3d12_framebuffer.dsv_handle.has_value() == depth_access.has_value(),
               "There must be either both a DSV handle and a depth target access, or neither");

        // TODO: Decide if every framebuffer should be a separate renderpass

        if(in_render_pass) {
            commands->EndRenderPass();
        }

        std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> render_target_descriptions;
        render_target_descriptions.reserve(render_target_accesses.size());

        for(uint32_t i = 0; i < render_target_accesses.size(); i++) {
            D3D12_RENDER_PASS_RENDER_TARGET_DESC desc{};
            desc.cpuDescriptor = d3d12_framebuffer.rtv_handles[i];
            desc.BeginningAccess = to_d3d12_beginning_access(render_target_accesses[i].begin);
            desc.EndingAccess = to_d3d12_ending_access(render_target_accesses[i].end);

            render_target_descriptions.emplace_back(desc);
        }

        if(depth_access) {
            D3D12_RENDER_PASS_DEPTH_STENCIL_DESC desc{};
            desc.cpuDescriptor = *d3d12_framebuffer.dsv_handle;
            desc.DepthBeginningAccess = to_d3d12_beginning_access(depth_access->begin);
            desc.DepthEndingAccess = to_d3d12_ending_access(depth_access->end);
            desc.StencilBeginningAccess = to_d3d12_beginning_access(depth_access->begin);
            desc.StencilEndingAccess = to_d3d12_ending_access(depth_access->end);

            commands->BeginRenderPass(static_cast<UINT>(render_target_descriptions.size()),
                                      render_target_descriptions.data(),
                                      &desc,
                                      D3D12_RENDER_PASS_FLAG_NONE);

        } else {
            commands->BeginRenderPass(static_cast<UINT>(render_target_descriptions.size()),
                                      render_target_descriptions.data(),
                                      nullptr,
                                      D3D12_RENDER_PASS_FLAG_NONE);
        }

        in_render_pass = true;

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = d3d12_framebuffer.width;
        viewport.Height = d3d12_framebuffer.height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(d3d12_framebuffer.width);
        scissor_rect.bottom = static_cast<LONG>(d3d12_framebuffer.height);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void D3D12RenderCommandList::set_pipeline_state(const RenderPipelineState& state) {
        MTR_SCOPE("D3D12RenderCommandList", "set_pipeline_state");

        const auto& d3d12_state = static_cast<const D3D12RenderPipelineState&>(state);

        if(current_render_pipeline_state == nullptr || current_render_pipeline_state->root_signature != d3d12_state.root_signature) {
            commands->SetGraphicsRootSignature(d3d12_state.root_signature.Get());
            is_render_material_bound = false;
        }

        commands->SetPipelineState(d3d12_state.pso.Get());

        current_render_pipeline_state = &d3d12_state;
    }

    void D3D12RenderCommandList::bind_render_resources(const BindGroup& bind_group) {
        MTR_SCOPE("D3D12RenderCommandList", "bind_render_resources");

        ENSURE(current_render_pipeline_state != nullptr, "Must bind a render pipeline before binding render resources");

        const auto& d3d12_bind_group = static_cast<const D3D12BindGroup&>(bind_group);
        for(const BoundResource<D3D12Buffer>& resource : d3d12_bind_group.used_buffers) {
            set_resource_state(*resource.resource, resource.states);
        }

        for(const BoundResource<D3D12Image>& resource : d3d12_bind_group.used_images) {
            set_resource_state(*resource.resource, resource.states);
        }

        if(current_descriptor_heap != d3d12_bind_group.heap) {
            commands->SetDescriptorHeaps(1, &d3d12_bind_group.heap);
            current_descriptor_heap = d3d12_bind_group.heap;
        }

        d3d12_bind_group.bind_to_graphics_signature(*commands.Get());

        is_render_material_bound = true;
    }

    void D3D12RenderCommandList::set_camera_idx(const uint32_t camera_idx) {
        MTR_SCOPE("D3D12RenderCommandList", "set_camera_idx");

        ENSURE(current_render_pipeline_state != nullptr, "Must bind a pipeline before setting the camera index");

        commands->SetGraphicsRoot32BitConstant(0, camera_idx, 0);
    }

    void D3D12RenderCommandList::bind_mesh_data(const MeshDataStore& mesh_data) {
        MTR_SCOPE("D3D12RenderCommandList", "bind_mesh_data");

        const auto& vertex_bindings = mesh_data.get_vertex_bindings();

        // If we have more than 16 vertex attributes, we probably have bigger problems
        std::array<D3D12_VERTEX_BUFFER_VIEW, 16> vertex_buffer_views;
        for(uint32_t i = 0; i < vertex_bindings.size(); i++) {
            const auto& binding = vertex_bindings[i];
            const auto* d3d12_buffer = static_cast<const D3D12Buffer*>(binding.buffer);

            set_resource_state(*d3d12_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            D3D12_VERTEX_BUFFER_VIEW view{};
            view.BufferLocation = d3d12_buffer->resource->GetGPUVirtualAddress() + binding.offset;
            view.SizeInBytes = d3d12_buffer->size - binding.offset;
            view.StrideInBytes = binding.vertex_size;

            vertex_buffer_views[i] = view;
        }

        commands->IASetVertexBuffers(0, static_cast<UINT>(vertex_bindings.size()), vertex_buffer_views.data());

        const auto& index_buffer = mesh_data.get_index_buffer();
        const auto& d3d12_index_buffer = static_cast<const D3D12Buffer&>(index_buffer);

        set_resource_state(d3d12_index_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        D3D12_INDEX_BUFFER_VIEW index_view{};
        index_view.BufferLocation = d3d12_index_buffer.resource->GetGPUVirtualAddress();
        index_view.SizeInBytes = index_buffer.size;
        index_view.Format = DXGI_FORMAT_R32_UINT;

        commands->IASetIndexBuffer(&index_view);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        is_mesh_data_bound = true;
    }

    void D3D12RenderCommandList::draw(const uint32_t num_indices, const uint32_t first_index, const uint32_t num_instances) {
        MTR_SCOPE("D3D12RenderCommandList", "draw");

        // ENSURE(is_render_material_bound, "Must bind material data to issue drawcalls");
        ENSURE(is_mesh_data_bound, "Must bind mesh data to issue drawcalls");
        ENSURE(current_render_pipeline_state != nullptr, "Must bind a render pipeline to issue drawcalls");

        commands->DrawIndexedInstanced(num_indices, num_instances, first_index, 0, 0);
    }

    RaytracingMesh D3D12RenderCommandList::build_acceleration_structure_for_mesh(const uint32_t num_vertices,
                                                                                 const uint32_t num_indices,
                                                                                 const uint32_t first_vertex,
                                                                                 const uint32_t first_index) {
        ENSURE(current_mesh_data != nullptr, "Must have mesh data bound before building acceleration structures out of it");

        const auto& index_buffer = current_mesh_data->get_index_buffer();
        const auto& d3d12_index_buffer = static_cast<const D3D12Buffer&>(index_buffer);

        const auto& vertex_buffer = *current_mesh_data->get_vertex_bindings()[0].buffer;
        const auto& d3d12_vertex_buffer = static_cast<const D3D12Buffer&>(vertex_buffer);

        const auto geom_desc = D3D12_RAYTRACING_GEOMETRY_DESC{
            .Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
            .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
            .Triangles = D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC{.Transform3x4 = 0,
                                                                  .IndexFormat = DXGI_FORMAT_R32_UINT,
                                                                  .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                                                                  .IndexCount = num_indices,
                                                                  .VertexCount = num_vertices,
                                                                  .IndexBuffer = d3d12_index_buffer.resource->GetGPUVirtualAddress() +
                                                                                 (first_index * sizeof(uint32_t)),
                                                                  .VertexBuffer = {.StartAddress = d3d12_vertex_buffer.resource
                                                                                                       ->GetGPUVirtualAddress() +
                                                                                                   (first_vertex * sizeof(BveVertex)),
                                                                                   .StrideInBytes = sizeof(BveVertex)}}};

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

        auto scratch_buffer = device->get_scratch_buffer(as_prebuild_info.ScratchDataSizeInBytes);

        const auto result_buffer_create_info = BufferCreateInfo{.name = "BLAS Result Buffer",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<uint32_t>(as_prebuild_info.ResultDataMaxSizeInBytes)};

        auto result_buffer = device->create_buffer(result_buffer_create_info);

        const auto& d3d12_scratch_buffer = static_cast<const D3D12Buffer&>(scratch_buffer);
        const auto& d3d12_result_buffer = static_cast<const D3D12Buffer&>(*result_buffer);

        const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
            .DestAccelerationStructureData = d3d12_result_buffer.resource->GetGPUVirtualAddress(),
            .Inputs = build_as_inputs,
            .ScratchAccelerationStructureData = d3d12_scratch_buffer.resource->GetGPUVirtualAddress()};

        DEFER(a, [&]() { device->return_scratch_buffer(std::move(scratch_buffer)); });

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(d3d12_result_buffer.resource.Get());
        commands->ResourceBarrier(1, &barrier);

        set_resource_state(d3d12_result_buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        return {std::move(result_buffer)};
    }

    RaytracingScene D3D12RenderCommandList::build_raytracing_scene(const std::vector<RaytracingObject>& objects) {
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

            const auto& d3d12_buffer = static_cast<const D3D12Buffer&>(*object.mesh->blas_buffer);
            desc.AccelerationStructure = d3d12_buffer.resource->GetGPUVirtualAddress();
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

        auto scratch_buffer = device->get_scratch_buffer(prebuild_info.ScratchDataSizeInBytes);

        const auto as_buffer_create_info = BufferCreateInfo{.name = "Raytracing Scene",
                                                            .usage = BufferUsage::RaytracingAccelerationStructure,
                                                            .size = static_cast<uint32_t>(prebuild_info.ResultDataMaxSizeInBytes)};
        auto as_buffer = device->create_buffer(as_buffer_create_info);
        const auto& d3d12_as_buffer = static_cast<const D3D12Buffer&>(*as_buffer);

        const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
            .DestAccelerationStructureData = d3d12_as_buffer.resource->GetGPUVirtualAddress(),
            .Inputs = as_inputs,
            .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress(),
        };

        DEFER(a, [&]() { device->return_staging_buffer(std::move(instance_buffer)); });
        DEFER(b, [&]() { device->return_scratch_buffer(std::move(scratch_buffer)); });

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(d3d12_as_buffer.resource.Get());
        commands->ResourceBarrier(1, &barrier);

        set_resource_state(d3d12_as_buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        return {.buffer = std::move(as_buffer)};
    }

    void D3D12RenderCommandList::prepare_for_submission() {
        MTR_SCOPE("D3D12RenderCommandList", "prepare_for_submission");
        if(in_render_pass) {
            commands->EndRenderPass();
        }

        D3D12ComputeCommandList::prepare_for_submission();
    }
} // namespace rhi

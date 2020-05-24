#pragma once

#include <string>

#include <d3d12.h>
#include <dxgi.h>
#include <spdlog/fmt/ostr.h>

#include "mesh_data_store.hpp"
#include "raytracing_structs.hpp"
#include "render_pipeline_state.hpp"
#include "resources.hpp"

interface ID3D12Object;

namespace rhi {
    struct RenderTargetBeginningAccess;
    struct RenderTargetEndingAccess;

    constexpr uint64_t FENCE_UNSIGNALED = 0;
    constexpr uint64_t CPU_FENCE_SIGNALED = 32;
    constexpr uint64_t GPU_FENCE_SIGNALED = 64;
    constexpr uint32_t FRAME_COMPLETE = 128;

    std::wstring to_wide_string(const std::string& string);

    std::string from_wide_string(const std::wstring& wide_string);

    void set_object_name(ID3D12Object& object, const std::string& name);

    DXGI_FORMAT to_dxgi_format(ImageFormat format);

    D3D12_BLEND to_d3d12_blend(BlendFactor factor);

    D3D12_BLEND_OP to_d3d12_blend_op(BlendOp op);

    D3D12_FILL_MODE to_d3d12_fill_mode(FillMode mode);

    D3D12_CULL_MODE to_d3d12_cull_mode(CullMode mode);

    D3D12_COMPARISON_FUNC to_d3d12_comparison_func(CompareOp op);

    D3D12_STENCIL_OP to_d3d12_stencil_op(StencilOp op);

    D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(PrimitiveType topology);

    D3D12_RENDER_PASS_BEGINNING_ACCESS to_d3d12_beginning_access(const RenderTargetBeginningAccess& access, bool is_color = true);

    D3D12_RENDER_PASS_ENDING_ACCESS to_d3d12_ending_access(const RenderTargetEndingAccess& access);

    std::string breadcrumb_to_string(D3D12_AUTO_BREADCRUMB_OP op);

    std::string allocation_type_to_string(D3D12_DRED_ALLOCATION_TYPE type);

    std::string breadcrumb_output_to_string(const D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT& breadcrumbs);

    std::string page_fault_output_to_string(const D3D12_DRED_PAGE_FAULT_OUTPUT& page_fault_output);

    RaytracingMesh build_acceleration_structure_for_meshes(ComPtr<ID3D12GraphicsCommandList4> commands,
                                                           RenderDevice& device,
                                                           const Buffer& vertex_buffer,
                                                           const Buffer& index_buffer,
                                                           const std::vector<Mesh>& meshes);
} // namespace rhi

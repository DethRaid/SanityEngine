#include "helpers.hpp"

#include <sstream>

#include <d3d11.h>

#include "core/align.hpp"
#include "core/ansi_colors.hpp"
#include "core/defer.hpp"
#include "framebuffer.hpp"
#include "render_device.hpp"

namespace renderer {

    Rx::WideString to_wide_string(const Rx::String& string) { return string.to_utf16(); }

    Rx::String from_wide_string(const Rx::WideString& wide_string) { return wide_string.to_utf8(); }

    void set_object_name(ID3D11DeviceChild* object, const Rx::String& name) {
        const auto wide_name = name.to_utf16();

        object->SetPrivateData(WKPDID_D3DDebugObjectNameW, static_cast<UINT>(wide_name.size() * sizeof(Uint16)), wide_name.data());
    }

    DXGI_FORMAT to_dxgi_format(const ImageFormat format) {
        switch(format) {
            case ImageFormat::Rgba32F:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;

            case ImageFormat::Depth32:
                return DXGI_FORMAT_D32_FLOAT;

            case ImageFormat::Depth24Stencil8:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;

            case ImageFormat::R32F:
                return DXGI_FORMAT_R32_FLOAT;

            case ImageFormat::Rg16F:
                return DXGI_FORMAT_R16G16_FLOAT;

            case ImageFormat::Rgba8:
                [[fallthrough]];
            default:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }

    D3D12_BLEND to_d3d11_blend(const BlendFactor factor) {
        switch(factor) {
            case BlendFactor::Zero:
                return D3D12_BLEND_ZERO;

            case BlendFactor::One:
                return D3D12_BLEND_ONE;

            case BlendFactor::SourceColor:
                return D3D12_BLEND_SRC_COLOR;

            case BlendFactor::InverseSourceColor:
                return D3D12_BLEND_INV_SRC_COLOR;

            case BlendFactor::SourceAlpha:
                return D3D12_BLEND_SRC_ALPHA;

            case BlendFactor::InverseSourceAlpha:
                return D3D12_BLEND_INV_SRC_ALPHA;

            case BlendFactor::DestinationColor:
                return D3D12_BLEND_DEST_COLOR;

            case BlendFactor::InverseDestinationColor:
                return D3D12_BLEND_INV_DEST_COLOR;

            case BlendFactor::DestinationAlpha:
                return D3D12_BLEND_DEST_ALPHA;

            case BlendFactor::InverseDestinationAlpha:
                return D3D12_BLEND_INV_DEST_ALPHA;

            case BlendFactor::SourceAlphaSaturated:
                return D3D12_BLEND_SRC_ALPHA_SAT;

            case BlendFactor::DynamicBlendFactor:
                return D3D12_BLEND_BLEND_FACTOR;

            case BlendFactor::InverseDynamicBlendFactor:
                return D3D12_BLEND_INV_BLEND_FACTOR;

            case BlendFactor::Source1Color:
                return D3D12_BLEND_SRC1_COLOR;

            case BlendFactor::InverseSource1Color:
                return D3D12_BLEND_INV_SRC1_COLOR;

            case BlendFactor::Source1Alpha:
                return D3D12_BLEND_SRC1_ALPHA;

            case BlendFactor::InverseSource1Alpha:
                return D3D12_BLEND_INV_SRC1_ALPHA;
        }

        return D3D12_BLEND_ZERO;
    }

    D3D12_BLEND_OP to_d3d11_blend_op(const BlendOp op) {
        switch(op) {
            case BlendOp::Add:
                return D3D12_BLEND_OP_ADD;

            case BlendOp::Subtract:
                return D3D12_BLEND_OP_SUBTRACT;

            case BlendOp::ReverseSubtract:
                return D3D12_BLEND_OP_REV_SUBTRACT;

            case BlendOp::Min:
                return D3D12_BLEND_OP_MIN;

            case BlendOp::Max:
                return D3D12_BLEND_OP_MAX;
        }

        return D3D12_BLEND_OP_ADD;
    }

    D3D12_FILL_MODE to_d3d11_fill_mode(const FillMode mode) {
        switch(mode) {
            case FillMode::Wireframe:
                return D3D12_FILL_MODE_WIREFRAME;

            case FillMode::Solid:
                [[fallthrough]];
            default:
                return D3D12_FILL_MODE_SOLID;
        }
    }

    D3D12_CULL_MODE to_d3d11_cull_mode(const CullMode mode) {
        switch(mode) {
            case CullMode::None:
                return D3D12_CULL_MODE_NONE;

            case CullMode::Front:
                return D3D12_CULL_MODE_FRONT;

            case CullMode::Back:
                [[fallthrough]];
            default:
                return D3D12_CULL_MODE_BACK;
        }
    }

    D3D12_COMPARISON_FUNC to_d3d11_comparison_func(const CompareOp op) {
        switch(op) {
            case CompareOp::Never:
                return D3D12_COMPARISON_FUNC_NEVER;

            case CompareOp::Less:
                return D3D12_COMPARISON_FUNC_LESS;

            case CompareOp::Equal:
                return D3D12_COMPARISON_FUNC_EQUAL;

            case CompareOp::NotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;

            case CompareOp::LessOrEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;

            case CompareOp::Greater:
                return D3D12_COMPARISON_FUNC_GREATER;

            case CompareOp::GreaterOrEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;

            case CompareOp::Always:
                [[fallthrough]];
            default:
                return D3D12_COMPARISON_FUNC_ALWAYS;
        }
    }

    D3D12_STENCIL_OP to_d3d11_stencil_op(const StencilOp op) {
        switch(op) {
            case StencilOp::Keep:
                return D3D12_STENCIL_OP_KEEP;

            case StencilOp::Zero:
                return D3D12_STENCIL_OP_ZERO;

            case StencilOp::Replace:
                return D3D12_STENCIL_OP_REPLACE;

            case StencilOp::Increment:
                return D3D12_STENCIL_OP_INCR;

            case StencilOp::IncrementAndSaturate:
                return D3D12_STENCIL_OP_INCR_SAT;

            case StencilOp::Decrement:
                return D3D12_STENCIL_OP_DECR;

            case StencilOp::DecrementAndSaturate:
                return D3D12_STENCIL_OP_DECR_SAT;

            case StencilOp::Invert:
                return D3D12_STENCIL_OP_INVERT;
        }

        return D3D12_STENCIL_OP_KEEP;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d11_primitive_topology_type(const PrimitiveType topology) {
        switch(topology) {
            case PrimitiveType::Points:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

            case PrimitiveType::Lines:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

            case PrimitiveType::Triangles:
                [[fallthrough]];
            default:
                return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        }
    }

    D3D12_RENDER_PASS_BEGINNING_ACCESS to_d3d11_beginning_access(const RenderTargetBeginningAccess& access, const bool is_color) {
        D3D12_RENDER_PASS_BEGINNING_ACCESS d3d11_access = {};

        switch(access.type) {
            case RenderTargetBeginningAccessType::Preserve:
                d3d11_access.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                break;

            case RenderTargetBeginningAccessType::Clear:
                d3d11_access.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                d3d11_access.Clear.ClearValue.Format = to_dxgi_format(access.format);
                if(is_color) {
                    d3d11_access.Clear.ClearValue.Color[0] = access.clear_color.x;
                    d3d11_access.Clear.ClearValue.Color[1] = access.clear_color.y;
                    d3d11_access.Clear.ClearValue.Color[2] = access.clear_color.z;
                    d3d11_access.Clear.ClearValue.Color[3] = access.clear_color.w;
                } else {
                    d3d11_access.Clear.ClearValue.DepthStencil.Depth = access.clear_color.x;
                    d3d11_access.Clear.ClearValue.DepthStencil.Stencil = 0;
                }
                break;

            case RenderTargetBeginningAccessType::Discard:
                d3d11_access.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                break;
        }

        return d3d11_access;
    }

    D3D12_RENDER_PASS_ENDING_ACCESS to_d3d11_ending_access(const RenderTargetEndingAccess& access) {
        D3D12_RENDER_PASS_ENDING_ACCESS d3d11_access{};

        switch(access.type) {
            case RenderTargetEndingAccessType::Preserve:
                d3d11_access.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                break;

            case RenderTargetEndingAccessType::Resolve:
                d3d11_access.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
                // TODO: Deal with this later
                break;

            case RenderTargetEndingAccessType::Discard:
                d3d11_access.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                break;
        }

        return d3d11_access;
    }

    RaytracableGeometry build_acceleration_structure_for_meshes(const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                                                RenderDevice& device,
                                                                const Buffer& vertex_buffer,
                                                                const Buffer& index_buffer,
                                                                const Rx::Vector<Mesh>& meshes) {

        Rx::Vector<D3D12_RAYTRACING_GEOMETRY_DESC> geom_descs;
        geom_descs.reserve(meshes.size());
        meshes.each_fwd([&](const Mesh& mesh) {
            const auto& [first_vertex, num_vertices, first_index, num_indices] = mesh;
            auto geom_desc = D3D12_RAYTRACING_GEOMETRY_DESC{.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
                                                            .Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
                                                            .Triangles = {.Transform3x4 = 0,
                                                                          .IndexFormat = DXGI_FORMAT_R32_UINT,
                                                                          .VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
                                                                          .IndexCount = num_indices,
                                                                          .VertexCount = num_vertices,
                                                                          .IndexBuffer = index_buffer.resource->GetGPUVirtualAddress() +
                                                                                         (first_index * sizeof(Uint32)),
                                                                          .VertexBuffer = {.StartAddress = vertex_buffer.resource
                                                                                                               ->GetGPUVirtualAddress(),
                                                                                           .StrideInBytes = sizeof(StandardVertex)}}};

            geom_descs.push_back(std::move(geom_desc));
        });

        const auto build_as_inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
            .Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
            .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
            .NumDescs = static_cast<UINT>(geom_descs.size()),
            .DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
            .pGeometryDescs = geom_descs.data()};

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_prebuild_info{};
        device.device5->GetRaytracingAccelerationStructurePrebuildInfo(&build_as_inputs, &as_prebuild_info);

        as_prebuild_info.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                        as_prebuild_info.ScratchDataSizeInBytes);
        as_prebuild_info.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                                          as_prebuild_info.ResultDataMaxSizeInBytes);

        auto scratch_buffer = device.get_scratch_buffer(static_cast<Uint32>(as_prebuild_info.ScratchDataSizeInBytes));

        const auto result_buffer_create_info = BufferCreateInfo{.name = "BLAS Result Buffer",
                                                                .usage = BufferUsage::RaytracingAccelerationStructure,
                                                                .size = static_cast<Uint32>(as_prebuild_info.ResultDataMaxSizeInBytes)};

        auto result_buffer = device.create_buffer(result_buffer_create_info);

        const auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
            .DestAccelerationStructureData = result_buffer->resource->GetGPUVirtualAddress(),
            .Inputs = build_as_inputs,
            .ScratchAccelerationStructureData = scratch_buffer.resource->GetGPUVirtualAddress()};

        DEFER(a, [&]() { device.return_scratch_buffer(std::move(scratch_buffer)); });

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(result_buffer->resource.Get());
        commands->ResourceBarrier(1, &barrier);

        return {std::move(result_buffer)};
    }

    void upload_data_with_staging_buffer(const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                         RenderDevice& device,
                                         ID3D12Resource* dst,
                                         const void* src,
                                         const Uint32 size,
                                         const Uint32 dst_offset) {
        auto staging_buffer = device.get_staging_buffer(size);
        memcpy(staging_buffer.mapped_ptr, src, size);

        commands->CopyBufferRegion(dst, dst_offset, staging_buffer.resource.Get(), 0, size);

        device.return_staging_buffer(std::move(staging_buffer));
    }

    ScopedD3DAnnotation::ScopedD3DAnnotation(ID3DUserDefinedAnnotation* annotation_in, const Rx::String& name) : annotation{annotation_in} {
        const auto wide_name = name.to_utf16();
        annotation->BeginEvent(reinterpret_cast<LPCWSTR>(wide_name.data()));
    }

    ScopedD3DAnnotation::ScopedD3DAnnotation(ID3D11DeviceContext* context, const Rx::String& name) {
        context->QueryInterface(&annotation);

        const auto wide_name = name.to_utf16();
        annotation->BeginEvent(reinterpret_cast<LPCWSTR>(wide_name.data()));
    }

    ScopedD3DAnnotation::~ScopedD3DAnnotation() {
        annotation->EndEvent();
    }

} // namespace renderer
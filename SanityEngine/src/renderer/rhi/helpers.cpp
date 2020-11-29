#include "helpers.hpp"

#include <sstream>

#include <d3d12.h>

#include "core/align.hpp"
#include "core/ansi_colors.hpp"
#include "core/defer.hpp"
#include "d3dx12.hpp"
#include "framebuffer.hpp"
#include "render_device.hpp"

namespace sanity::engine::renderer {
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

    D3D12_BLEND to_d3d12_blend(const BlendFactor factor) {
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

    D3D12_BLEND_OP to_d3d12_blend_op(const BlendOp op) {
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

    D3D12_FILL_MODE to_d3d12_fill_mode(const FillMode mode) {
        switch(mode) {
            case FillMode::Wireframe:
                return D3D12_FILL_MODE_WIREFRAME;

            case FillMode::Solid:
                [[fallthrough]];
            default:
                return D3D12_FILL_MODE_SOLID;
        }
    }

    D3D12_CULL_MODE to_d3d12_cull_mode(const CullMode mode) {
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

    D3D12_COMPARISON_FUNC to_d3d12_comparison_func(const CompareOp op) {
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

    D3D12_STENCIL_OP to_d3d12_stencil_op(const StencilOp op) {
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

    D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(const PrimitiveType topology) {
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

    D3D12_RENDER_PASS_BEGINNING_ACCESS to_d3d12_beginning_access(const RenderTargetBeginningAccess& access, const bool is_color) {
        D3D12_RENDER_PASS_BEGINNING_ACCESS d3d12_access = {};

        switch(access.type) {
            case RenderTargetBeginningAccessType::Preserve:
                d3d12_access.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                break;

            case RenderTargetBeginningAccessType::Clear:
                d3d12_access.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                d3d12_access.Clear.ClearValue.Format = to_dxgi_format(access.format);
                if(is_color) {
                    d3d12_access.Clear.ClearValue.Color[0] = access.clear_color.x;
                    d3d12_access.Clear.ClearValue.Color[1] = access.clear_color.y;
                    d3d12_access.Clear.ClearValue.Color[2] = access.clear_color.z;
                    d3d12_access.Clear.ClearValue.Color[3] = access.clear_color.w;
                } else {
                    d3d12_access.Clear.ClearValue.DepthStencil.Depth = access.clear_color.x;
                    d3d12_access.Clear.ClearValue.DepthStencil.Stencil = 0;
                }
                break;

            case RenderTargetBeginningAccessType::Discard:
                d3d12_access.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                break;
        }

        return d3d12_access;
    }

    D3D12_RENDER_PASS_ENDING_ACCESS to_d3d12_ending_access(const RenderTargetEndingAccess& access) {
        D3D12_RENDER_PASS_ENDING_ACCESS d3d12_access{};

        switch(access.type) {
            case RenderTargetEndingAccessType::Preserve:
                d3d12_access.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                break;

            case RenderTargetEndingAccessType::Resolve:
                d3d12_access.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
                // TODO: Deal with this later
                break;

            case RenderTargetEndingAccessType::Discard:
                d3d12_access.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                break;
        }

        return d3d12_access;
    }

    Rx::String resource_state_to_string(const D3D12_RESOURCE_STATES state) {
        switch(state) {
            case D3D12_RESOURCE_STATE_COMMON:
                return "COMMON";
        	
            case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
                return "VERTEX_AND_CONSTANT_BUFFER";
        	
            case D3D12_RESOURCE_STATE_INDEX_BUFFER:
                return "INDEX_BUFFER";
        	
            case D3D12_RESOURCE_STATE_RENDER_TARGET:
                return "RENDER_TARGET";
        	
            case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
                return "UNORDERED_ACCESS";
        	
            case D3D12_RESOURCE_STATE_DEPTH_WRITE:
                return "DEPTH_WRITE";
        	
            case D3D12_RESOURCE_STATE_DEPTH_READ:
                return "DEPTH_READ";
        	
            case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
                return "NON_PIXEL_SHADER_RESOURCE";
        	
            case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
                return "PIXEL_SHADER_RESOURCE";
        	
            case D3D12_RESOURCE_STATE_STREAM_OUT:
                return "STREAM_OUT";
        	
            case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
                return "INDIRECT_ARGUMENT";
        	
            case D3D12_RESOURCE_STATE_COPY_DEST:
                return "COPY_DEST";
        	
            case D3D12_RESOURCE_STATE_COPY_SOURCE:
                return "COPY_SOURCE";
        	
            case D3D12_RESOURCE_STATE_RESOLVE_DEST:
                return "RESOLVE_DEST";
        	
            case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
                return "RESOLVE_SOURCE";
        	
            case D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
                return "RAYTRACING_ACCELERATION_STRUCTURE";
        	
            case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
                return "SHADING_RATE_SOURCE";
        	
            case D3D12_RESOURCE_STATE_GENERIC_READ:
                return "GENERIC_READ";
        	
            default:
                return "<UNKNOWN>";
        }
    }

    Rx::String breadcrumb_output_to_string(const D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1& breadcrumbs) {
        Rx::String breadcrumb_output_string;

        const auto* cur_node = breadcrumbs.pHeadAutoBreadcrumbNode;

        while(cur_node != nullptr) {
            const auto command_list_name = [&]() -> Rx::String {
                if(cur_node->pCommandListDebugNameW != nullptr) {
                    return Rx::WideString{reinterpret_cast<const Uint16*>(cur_node->pCommandListDebugNameW)}.to_utf8();

                } else if(cur_node->pCommandListDebugNameA != nullptr) {
                    return cur_node->pCommandListDebugNameA;

                } else {
                    return "Unknown command list";
                }
            }();

            const auto command_queue_name = cur_node->pCommandQueueDebugNameW != nullptr ?
                                                Rx::WideString{reinterpret_cast<const Uint16*>(cur_node->pCommandQueueDebugNameW)}
                                                    .to_utf8() :
                                                "Unknown command queue";

            const auto last_breadcrumb_idx = *cur_node->pLastBreadcrumbValue;
            const auto& breadcrumb = cur_node->pCommandHistory[last_breadcrumb_idx];
            breadcrumb_output_string += Rx::String::
                format("Command list %s, executing on command queue %s, has completed on %d render operations",
                       command_list_name,
                       command_queue_name,
                       last_breadcrumb_idx);

            if(breadcrumb != D3D12_AUTO_BREADCRUMB_OP_SETMARKER) {
                breadcrumb_output_string += Rx::String::format("\nMost recent operation: %s%s%s",
                                                               colors::INCOMPLETE_BREADCRUMB,
                                                               breadcrumb_to_string(breadcrumb),
                                                               colors::DEFAULT_CONSOLE_COLOR);
            }

            if(cur_node->BreadcrumbCount > 0) {
                for(Uint32 i = 0; i < cur_node->BreadcrumbCount; i++) {
                    const char* color = colors::DEFAULT_CONSOLE_COLOR;
                    if(i < last_breadcrumb_idx) {
                        color = colors::COMPLETED_BREADCRUMB;

                    } else if(i == last_breadcrumb_idx) {
                        color = colors::INCOMPLETE_BREADCRUMB;

                    } else {
                        color = colors::DEFAULT_CONSOLE_COLOR;
                    }

                    breadcrumb_output_string += Rx::String::format("\n\t%s%s", color, breadcrumb_to_string(cur_node->pCommandHistory[i]));

                    if(cur_node->BreadcrumbContextsCount > 0) {
                        for(Uint32 context_idx = 0; context_idx < cur_node->BreadcrumbContextsCount; context_idx++) {
                            const auto& cur_breadcrumb_context = cur_node->pBreadcrumbContexts[context_idx];
                            if(cur_breadcrumb_context.BreadcrumbIndex == i) {
                                breadcrumb_output_string += Rx::String::format("\n\t\t%s%s",
                                                                               colors::CONTEXT_LABEL,
                                                                               Rx::WideString{reinterpret_cast<const Uint16*>(
                                                                                                  cur_breadcrumb_context.pContextString)}
                                                                                   .to_utf8());
                                break;
                            }
                        }
                    }
                    breadcrumb_output_string += "\033[m";
                }
            }

            breadcrumb_output_string += "\n\033[40m";

            cur_node = cur_node->pNext;
        }

        return breadcrumb_output_string;
    }

    void print_allocation_chain(const D3D12_DRED_ALLOCATION_NODE1* head, std::stringstream& ss) {
        const auto* allocation = head;
        while(allocation != nullptr) {
            ss << "\n\t";
            if(allocation->ObjectNameA != nullptr) {
                ss << allocation->ObjectNameA;

            } else if(allocation->ObjectNameW != nullptr) {
                const auto& name = Rx::WideString{reinterpret_cast<const Uint16*>(allocation->ObjectNameW)}.to_utf8();
                ss << name.data();

            } else {
                ss << "Unnamed allocation";
            }
            ss << " (" << allocation_type_to_string(allocation->AllocationType).data() << ")";

            allocation = allocation->pNext;
        }
    }

    Rx::String page_fault_output_to_string(const D3D12_DRED_PAGE_FAULT_OUTPUT1& page_fault_output) {
        std::stringstream ss;

        ss << "Page fault at GPU virtual address " << page_fault_output.PageFaultVA;

        if(page_fault_output.pHeadExistingAllocationNode != nullptr) {
            ss << "\nActive allocations:";
            print_allocation_chain(page_fault_output.pHeadExistingAllocationNode, ss);
        }

        if(page_fault_output.pHeadRecentFreedAllocationNode != nullptr) {
            ss << "\nRecently freed allocations:";
            print_allocation_chain(page_fault_output.pHeadRecentFreedAllocationNode, ss);
        }

        return ss.str().c_str();
    }

    RaytracableGeometry build_acceleration_structure_for_meshes(ID3D12GraphicsCommandList4* commands,
                                                                RenderBackend& device,
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

            geom_descs.push_back(Rx::Utility::move(geom_desc));
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

        DEFER(a, [&]() { device.return_scratch_buffer(Rx::Utility::move(scratch_buffer)); });

        commands->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(result_buffer->resource.Get());
        commands->ResourceBarrier(1, &barrier);

        return {Rx::Utility::move(result_buffer)};
    }

    void upload_data_with_staging_buffer(ID3D12GraphicsCommandList* commands,
                                         RenderBackend& device,
                                         ID3D12Resource* dst,
                                         const void* src,
                                         const Uint32 size,
                                         const Uint32 dst_offset) {
        auto staging_buffer = device.get_staging_buffer(size);
        memcpy(staging_buffer.mapped_ptr, src, size);

        commands->CopyBufferRegion(dst, dst_offset, staging_buffer.resource.Get(), 0, size);

        device.return_staging_buffer(Rx::Utility::move(staging_buffer));
    }

    Rx::String breadcrumb_to_string(const D3D12_AUTO_BREADCRUMB_OP op) {
        switch(op) {
            case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
                return "Set marker";

            case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
                return "Begin event";

            case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
                return "End event";

            case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
                return "Draw instanced";

            case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
                return "Draw indexed instanced";

            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
                return "Execute indirect";

            case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
                return "Dispatch";

            case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
                return "Copy buffer region";

            case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
                return "Copy texture region";

            case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
                return "Copy resource";

            case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
                return "Copy tiles";

            case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
                return "Resolve subresource";

            case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
                return "Clear render target view";

            case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
                return "Clear unordered access view";

            case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
                return "Clear depth stencil view";

            case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
                return "Resource barrier";

            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
                return "Execute bundle";

            case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
                return "Present";

            case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
                return "Resolve query data";

            case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
                return "Begin submission";

            case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
                return "End submission";

            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
                return "Decode frame";

            case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
                return "Process frames";

            case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
                return "Atomic copy buffer uint";

            case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
                return "Atomic copy buffer uint64";

            case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
                return "Resolve subresource region";

            case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
                return "Write buffer immediate";

            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
                return "Decode frame 1";

            case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
                return "Set protected resource session";

            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
                return "Decode frame 2";

            case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
                return "Process frames 1";

            case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
                return "Build raytracing acceleration structure";

            case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
                return "Emit raytracing acceleration structure post build info";

            case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
                return "Copy raytracing acceleration structure";

            case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
                return "Dispatch rays";

            case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
                return "Initialize meta command";

            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
                return "Execute meta command";

            case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
                return "Estimate motion";

            case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
                return "Resolve motion vector heap";

            case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
                return "Set pipeline state 1";

            case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
                return "Initialize extension command";

            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
                return "Execute extension command";

            case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
                return "Dispatch mesh";

            default:
                return "Unknown breadcrumb";
        }
    }

    Rx::String allocation_type_to_string(const D3D12_DRED_ALLOCATION_TYPE type) {
        switch(type) {
            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:
                return "Command queue";

            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:
                return "Command allocator";

            case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:
                return "Pipeline state";

            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:
                return "Command list";

            case D3D12_DRED_ALLOCATION_TYPE_FENCE:
                return "Fence";

            case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:
                return "Descriptor heap";

            case D3D12_DRED_ALLOCATION_TYPE_HEAP:
                return "Heap";

            case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:
                return "Query heap";

            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE:
                return "Command signature";

            case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY:
                return "Pipeline library";

            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER:
                return "Video decoder";

            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR:
                return "Video processor";

            case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:
                return "Resource";

            case D3D12_DRED_ALLOCATION_TYPE_PASS:
                return "Pass";

            case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION:
                return "Crypto session";

            case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY:
                return "Crypto session policy";

            case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION:
                return "Protected resource session";

            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP:
                return "Video decoder heap";

            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL:
                return "Command pool";

            case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER:
                return "Command recorder";

            case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT:
                return "State object";

            case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND:
                return "Meta command";

            case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP:
                return "Scheduling group";

            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR:
                return "Video motion estimator";

            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP:
                return "Motion vector heap";

            case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND:
                return "Video extension command";

            case D3D12_DRED_ALLOCATION_TYPE_INVALID:
                return "Invalid";

            default:
                return "Unknown object type";
        }
    }
} // namespace renderer
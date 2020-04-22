#include "helpers.hpp"

#include <d3d12.h>

namespace rhi {

    std::wstring to_wide_string(const std::string& string) {
        const int wide_string_length = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, nullptr, 0);
        wchar_t* wide_char_string = new wchar_t[wide_string_length];
        MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wide_char_string, wide_string_length);

        std::wstring wide_string{wide_char_string};

        delete[] wide_char_string;

        return wide_string;
    }

    void set_object_name(ID3D12Object& object, const std::string& name) {
        const auto wide_name = to_wide_string(name);

        object.SetName(reinterpret_cast<LPCWSTR>(wide_name.c_str()));
    }

    DXGI_FORMAT to_dxgi_format(const ImageFormat format) {
        switch(format) {
            case ImageFormat::Rgba32F:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;

            case ImageFormat::Depth32:
                return DXGI_FORMAT_D32_FLOAT;

            case ImageFormat::Depth24Stencil8:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;

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
} // namespace render
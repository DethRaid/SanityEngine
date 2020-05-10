#pragma once

#include <array>
#include <optional>

#include <d3d12.h>
#include <wrl/client.h>

#include "resources.hpp"

using Microsoft::WRL::ComPtr;

namespace rhi {
    enum class PrimitiveType { Points, Lines, Triangles };

    enum class BlendFactor {
        Zero,
        One,
        SourceColor,
        InverseSourceColor,
        SourceAlpha,
        InverseSourceAlpha,
        DestinationColor,
        InverseDestinationColor,
        DestinationAlpha,
        InverseDestinationAlpha,
        SourceAlphaSaturated,
        DynamicBlendFactor,
        InverseDynamicBlendFactor,
        Source1Color,
        InverseSource1Color,
        Source1Alpha,
        InverseSource1Alpha,
    };

    enum class BlendOp { Add, Subtract, ReverseSubtract, Min, Max };

    struct RenderTargetBlendState {
        bool enabled{false};

        BlendFactor source_color_blend_factor{BlendFactor::SourceAlpha};
        BlendFactor destination_color_blend_factor{BlendFactor::InverseSourceAlpha};
        BlendOp color_blend_op{BlendOp::Add};

        BlendFactor source_alpha_blend_factor{BlendFactor::SourceAlpha};
        BlendFactor destination_alpha_blend_factor{BlendFactor::InverseSourceAlpha};
        BlendOp alpha_blend_op{BlendOp::Add};
    };

    struct BlendState {
        bool enable_alpha_to_coverage{false};

        std::array<RenderTargetBlendState, 8> render_target_blends;
    };

    enum class FillMode {
        Wireframe,
        Solid,
    };

    enum class CullMode { None, Front, Back };

    struct RasterizerState {
        FillMode fill_mode{FillMode::Solid};

        CullMode cull_mode{CullMode::Back};

        bool front_face_counter_clockwise{false};

        float depth_bias{0};
        float max_depth_bias{0};
        float slope_scaled_depth_bias{0};

        uint32_t num_msaa_samples{1};

        bool enable_line_antialiasing{false};

        bool enable_conservative_rasterization{false};
    };

    enum class CompareOp {
        Never,
        Less,
        Equal,
        NotEqual,
        LessOrEqual,
        Greater,
        GreaterOrEqual,
        Always,
    };

    enum class StencilOp {
        Keep,
        Zero,
        Replace,
        Increment,
        IncrementAndSaturate,
        Decrement,
        DecrementAndSaturate,
        Invert,
    };

    struct StencilState {
        StencilOp fail_op{StencilOp::Keep};
        StencilOp depth_fail_op{StencilOp::Keep};
        StencilOp pass_op{StencilOp::Replace};
        CompareOp compare_op{CompareOp::Always};
    };

    struct DepthStencilState {
        bool enable_depth_test{true};
        bool enable_depth_write{true};
        CompareOp depth_func{CompareOp::Less};

        bool enable_stencil_test{false};
        uint8_t stencil_read_mask{0xFF};
        uint8_t stencil_write_mask{0xFF};
        StencilState front_face{};
        StencilState back_face{};
    };

    struct RenderPipelineStateCreateInfo {
        std::string name;

        std::vector<uint8_t> vertex_shader{};

        std::optional<std::vector<uint8_t>> pixel_shader{std::nullopt};

        BlendState blend_state{};

        RasterizerState rasterizer_state{};

        DepthStencilState depth_stencil_state{};

        PrimitiveType primitive_type{PrimitiveType::Triangles};

        /*!
         * \brief Formats of any render target this render pipeline outputs to
         */
        std::vector<ImageFormat> render_target_formats;

        /*!
         * \brief Format of the depth/stencil target that this render pipeline outputs to
         */
        std::optional<ImageFormat> depth_stencil_format;
    };

    struct RenderPipelineState {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };

    enum class BespokePipelineType {
        /*!
         * \brief The pipeline will output to the backbuffer
         */
        BackbufferOutput,
    };
} // namespace rhi

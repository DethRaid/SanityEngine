#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "core/types.hpp"
#include "rhi/render_device.hpp"

using Microsoft::WRL::ComPtr;

namespace sanity::engine::renderer {
    class RenderBackend;

    /*!
     * \brief Abstraction over AMD's single-pass denoiser
     */
    class SinglePassDownsampler {
    public:
        static const Uint32 MIP_COUNT_ROOT_CONSTANT_OFFSET = 0;
        static const Uint32 NUM_WORK_GROUPS_ROOT_CONSTANT_OFFSET = 1;
        static const Uint32 OFFSET_ROOT_CONSTANT_OFFSET = 2;
        static const Uint32 INVERSE_SIZE_ROOT_CONSTANT_OFFSET = 4;

    	static const Uint32 ROOT_CONSTANTS_INDEX = 0;
        static const Uint32 GLOBAL_COUNTER_BUFFER_INDEX = 1;
        static const Uint32 OUTPUT_MIP_6_INDEX = 2;
        static const Uint32 OUTPUT_MIPS_INDEX = 3;
        static const Uint32 INPUT_MIP_0_INDEX = 4;

        [[nodiscard]] static SinglePassDownsampler Create(RenderBackend& backend);

        void generate_mip_chain_for_texture(ID3D12Resource* texture, ID3D12GraphicsCommandList4* cmds);

    private:
        ComPtr<ID3D12RootSignature> root_signature;

        ComPtr<ID3D12PipelineState> pipeline;

        RenderBackend* backend;

        explicit SinglePassDownsampler(ComPtr<ID3D12RootSignature> root_signature_in,
                                    ComPtr<ID3D12PipelineState> pipeline_in,
                                    RenderBackend& backend_in);

        [[nodiscard]] DescriptorTableHandle init_mip_output_descriptors(ID3D12Resource* texture,
                                                                        const ComPtr<ID3D12Device>& device,
                                                                        Uint32 num_mips) const;
    };
} // namespace sanity::engine::renderer

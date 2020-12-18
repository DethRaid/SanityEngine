#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "core/types.hpp"

using Microsoft::WRL::ComPtr;

namespace sanity::engine::renderer {
    class RenderBackend;

    /*!
     * \brief Abstraction over AMD's single-pass denoiser
     */
    class SinglePassDenoiser {
    public:
        static const Uint32 MIP_COUNT_ROOT_CONSTANT_OFFSET = 0;
        static const Uint32 NUM_WORK_GROUPS_ROOT_CONSTANT_OFFSET = 1;
        static const Uint32 OFFSET_X_ROOT_CONSTANT_OFFSET = 2;
        static const Uint32 OFFSET_Y_ROOT_CONSTANT_OFFSET = 3;
    	
        [[nodiscard]] static SinglePassDenoiser Create(RenderBackend& backend);

        void generate_mip_chain_for_texture(ID3D12Resource* texture, ID3D12GraphicsCommandList* cmds);

    private:
        ComPtr<ID3D12RootSignature> root_signature;

        ComPtr<ID3D12PipelineState> pipeline;

        explicit SinglePassDenoiser(ComPtr<ID3D12RootSignature> root_signature_in, ComPtr<ID3D12PipelineState> pipeline_in);
    };
} // namespace sanity::engine::renderer

#pragma once

#include <d3d12.h>

#include "core/types.hpp"
#include "rhi/render_backend.hpp"

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
        static const Uint32 DESCRIPTOR_TABLE_INDEX = 2;

        [[nodiscard]] static SinglePassDownsampler Create(RenderBackend& backend);

        /*!
         * @brief Generates the full mip chain for the given texture
         *
         * Texture must be in the UNORDERED_ACCESS state
         *
         * Texture mip 0 must have useful data
         *
         * @param texture Texture to generate mips for
         * @param cmds Command list ot use
         */
        void generate_mip_chain_for_texture(ID3D12Resource* texture, ID3D12GraphicsCommandList2* cmds) const;

    private:
        ComPtr<ID3D12RootSignature> root_signature;

        ComPtr<ID3D12PipelineState> pipeline;

        RenderBackend* backend;

        explicit SinglePassDownsampler(ComPtr<ID3D12RootSignature> root_signature_in,
                                       ComPtr<ID3D12PipelineState> pipeline_in,
                                       RenderBackend& backend_in);

        [[nodiscard]] DescriptorRange fill_descriptor_table(ID3D12Resource* texture, ID3D12Device* device, Uint32 num_mips) const;
    };
} // namespace sanity::engine::renderer

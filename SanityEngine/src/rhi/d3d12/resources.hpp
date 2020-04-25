#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "../resources.hpp"

using Microsoft::WRL::ComPtr;

namespace D3D12MA {
    class Allocation;
}

namespace rhi {
    struct D3D12Buffer : Buffer {
        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        void* mapped_ptr{nullptr};
    };

    struct D3D12StagingBuffer : D3D12Buffer {
        void* ptr{nullptr};
    };

    struct D3D12Image : Image {
        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        DXGI_FORMAT format;
    };
} // namespace rhi

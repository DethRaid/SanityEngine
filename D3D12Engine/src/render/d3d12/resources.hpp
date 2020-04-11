#pragma once
#include "../resources.hpp"

#include <wrl/client.h>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;

namespace D3D12MA {
    class Allocation;
}

namespace render{
    struct D3D12Buffer : Buffer {
        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;
    };

    struct D3D12Image : Image {
        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        DXGI_FORMAT format;
    };
}

#pragma once

#include <DirectXMath.h>

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;

struct TransformComponent {
    XMFLOAT4 position;

    XMFLOAT3 rotation;

    XMFLOAT3 scale;
};

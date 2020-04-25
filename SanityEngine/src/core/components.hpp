#pragma once

#include <DirectXMath.h>

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;

struct TransformComponent {
    XMFLOAT4 position;

    XMFLOAT3 rotation;

    XMFLOAT3 scale;

    [[nodiscard]] __forceinline XMFLOAT3 get_forward_vector() const;
};

inline XMFLOAT3 TransformComponent::get_forward_vector() const {
    const auto cos_pitch = cos(rotation.x);

    return {cos_pitch * sin(rotation.z), cos_pitch * cos(rotation.z), sin(rotation.z)};
}

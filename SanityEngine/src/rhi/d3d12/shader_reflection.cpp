#include "shader_reflection.hpp"

#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

D3D12_ROOT_SIGNATURE_DESC1 get_compute_shader_root_signature(const std::vector<uint8_t>& compute_shader) {
    ComPtr<ID3D12ShaderReflection> reflection;
    D3DReflect(compute_shader.data(), compute_shader.size(), IID_PPV_ARGS(reflection.GetAddressOf()));

    D3D12_SHADER_DESC desc;
    reflection->GetDesc(&desc);

    
}
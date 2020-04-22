#pragma once

#include <vector>

#include <d3d12.h>
#include <stdint.h>

D3D12_ROOT_SIGNATURE_DESC1 get_compute_shader_root_signature(const std::vector<uint8_t>& compute_shader);

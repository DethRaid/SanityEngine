#pragma once

#include "../resources.hpp"
#include <dxgi.h>

class ID3D12Object;

constexpr uint64_t FENCE_UNSIGNALED = 0;
constexpr uint64_t CPU_FENCE_SIGNALED = 32;
constexpr uint64_t GPU_FENCE_SIGNALED = 64;

namespace rx {
    struct string;
}

void set_object_name(ID3D12Object& object, const std::string& name);

DXGI_FORMAT to_dxgi_format(render::ImageFormat format);

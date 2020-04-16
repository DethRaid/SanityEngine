#pragma once

#include <string>

#include <dxgi.h>

#include "../resources.hpp"

class ID3D12Object;

constexpr uint64_t FENCE_UNSIGNALED = 0;
constexpr uint64_t CPU_FENCE_SIGNALED = 32;
constexpr uint64_t GPU_FENCE_SIGNALED = 64;

std::wstring to_wide_string(const std::string& string);

void set_object_name(ID3D12Object& object, const std::string& name);

DXGI_FORMAT to_dxgi_format(render::ImageFormat format);

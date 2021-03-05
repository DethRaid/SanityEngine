// Use ifndef because dxc doesn't support #pragma once https://github.com/microsoft/DirectXShaderCompiler/issues/676
#ifndef INTEROP_HPP
#define INTEROP_HPP

#if __cplusplus
#include "core/types.hpp"
#include "glm/glm.hpp"
#include "renderer/rhi/resources.hpp"

// ReSharper disable CppInconsistentNaming

using uint = Uint32;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using float3 = glm::vec3;
using float4 = glm::vec4;

using float4x4 = glm::mat4;

// ReSharper restore CppInconsistentNaming

static_assert(sizeof(sanity::engine::renderer::TextureHandle) == sizeof(uint));
static_assert(sizeof(sanity::engine::renderer::BufferHandle) == sizeof(uint));
#endif
#endif

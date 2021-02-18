#pragma once

#include "core/types.hpp"   

constexpr Uint32 AMD_PCI_VENDOR_ID = 0x1022;
constexpr Uint32 INTEL_PCI_VENDOR_ID = 0x8086;
constexpr Uint32 NVIDIA_PCI_VENDOR_ID = 0x10DE;

constexpr Uint32 STATIC_MESH_VERTEX_BUFFER_SIZE = 64 << 20;
constexpr Uint32 STATIC_MESH_INDEX_BUFFER_SIZE = 64 << 20;

constexpr Uint32 MAX_NUM_CAMERAS = 256;
constexpr Uint32 MAX_NUM_BUFFERS = 65536;
constexpr Uint32 MAX_NUM_TEXTURES = 65536;

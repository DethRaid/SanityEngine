#pragma once

#include <stdint.h>

constexpr bool RX_ITERATION_CONTINUE = true;
constexpr bool RX_ITERATION_STOP = false;

constexpr uint32_t AMD_PCI_VENDOR_ID = 0x1022;
constexpr uint32_t INTEL_PCI_VENDOR_ID = 0x8086;
constexpr uint32_t NVIDIA_PCI_VENDOR_ID = 0x10DE;

constexpr uint32_t STATIC_MESH_VERTEX_BUFFER_SIZE = 64 << 20;
constexpr uint32_t STATIC_MESH_INDEX_BUFFER_SIZE = 64 << 20;

constexpr uint32_t MAX_NUM_CAMERAS = 256;
constexpr uint32_t MAX_NUM_TEXTURES = 65536;

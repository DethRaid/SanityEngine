#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "core/types.hpp"
#include "d3dx12.hpp"
#include "rx/core/vector.h"

using Microsoft::WRL::ComPtr;

namespace sanity::engine::renderer {

    struct __declspec(uuid("8FE90F37-09FE-4D01-8E3F-65A8ABC4349F")) DescriptorRange {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        Uint32 table_size;
    };

    class DescriptorAllocator {
    public:
        DescriptorAllocator(ComPtr<ID3D12DescriptorHeap> heap_in, UINT descriptor_size_in);

        DescriptorAllocator(const DescriptorAllocator& other) = delete;
        DescriptorAllocator& operator=(const DescriptorAllocator& other) = delete;

        DescriptorAllocator(DescriptorAllocator&& old) noexcept;
        DescriptorAllocator& operator=(DescriptorAllocator&& old) noexcept;

        ~DescriptorAllocator() = default;

        [[nodiscard]] DescriptorRange allocate_descriptors(Uint32 num_descriptors);

        void free_descriptor_range(DescriptorRange handle);

        [[nodiscard]] UINT get_descriptor_size() const;

        [[nodiscard]] ID3D12DescriptorHeap* get_heap() const;

    private:
        ComPtr<ID3D12DescriptorHeap> heap;

        UINT descriptor_size;

        INT next_free_descriptor{0};

        Rx::Vector<DescriptorRange> available_ranges;
    };
} // namespace sanity::engine::renderer

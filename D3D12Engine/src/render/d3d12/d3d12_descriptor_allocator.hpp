#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "rx/core/vector.h"
using Microsoft::WRL::ComPtr;

namespace render {
    class D3D12DescriptorAllocator {
    public:
        D3D12DescriptorAllocator(rx::memory::allocator& allocator, ComPtr<ID3D12DescriptorHeap> heap_in, UINT descriptor_size_in);

        D3D12DescriptorAllocator(const D3D12DescriptorAllocator& other) = delete;
        D3D12DescriptorAllocator& operator=(const D3D12DescriptorAllocator& other) = delete;

        D3D12DescriptorAllocator(D3D12DescriptorAllocator&& old) noexcept;
        D3D12DescriptorAllocator& operator=(D3D12DescriptorAllocator&& old) noexcept;

        ~D3D12DescriptorAllocator() = default;

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_next_free_descriptor();

        void return_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    private:
        ComPtr<ID3D12DescriptorHeap> heap;

        UINT descriptor_size;

        INT next_free_descriptor{0};

        rx::vector<D3D12_CPU_DESCRIPTOR_HANDLE> available_handles;
    };
} // namespace render

#pragma once

#include <vector>

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace rhi {
    class DescriptorAllocator {
    public:
        DescriptorAllocator(ComPtr<ID3D12DescriptorHeap> heap_in, UINT descriptor_size_in);

        DescriptorAllocator(const DescriptorAllocator& other) = delete;
        DescriptorAllocator& operator=(const DescriptorAllocator& other) = delete;

        DescriptorAllocator(DescriptorAllocator&& old) noexcept;
        DescriptorAllocator& operator=(DescriptorAllocator&& old) noexcept;

        ~DescriptorAllocator() = default;

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_next_free_descriptor();

        void return_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    private:
        ComPtr<ID3D12DescriptorHeap> heap;

        UINT descriptor_size;

        INT next_free_descriptor{0};

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> available_handles;
    };
} // namespace render
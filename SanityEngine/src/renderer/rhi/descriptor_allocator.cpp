#include "descriptor_allocator.hpp"

#include "d3dx12.hpp"

namespace sanity::engine::renderer {
    DescriptorAllocator::DescriptorAllocator(ComPtr<ID3D12DescriptorHeap> heap_in, const UINT descriptor_size_in)
        : heap{Rx::Utility::move(heap_in)}, descriptor_size{descriptor_size_in} {}

    DescriptorAllocator::DescriptorAllocator(DescriptorAllocator&& old) noexcept
        : heap{Rx::Utility::move(old.heap)},
          descriptor_size{old.descriptor_size},
          next_free_descriptor{old.next_free_descriptor},
          available_ranges{Rx::Utility::move(old.available_ranges)} {}

    DescriptorAllocator& DescriptorAllocator::operator=(DescriptorAllocator&& old) noexcept {
        heap = Rx::Utility::move(old.heap);
        descriptor_size = old.descriptor_size;
        next_free_descriptor = old.next_free_descriptor;
        available_ranges = Rx::Utility::move(old.available_ranges);

        return *this;
    }

    DescriptorRange DescriptorAllocator::allocate_descriptors(const Uint32 num_descriptors) {
        RX_ASSERT(num_descriptors > 0, "num_descriptors must be greater than 0!");

        Size best_match_idx = available_ranges.size();
        Uint32 waste_in_best_match = 0xFFFFFFFF;
        for(Uint32 i = 0; i < available_ranges.size(); i++) {
            const auto& range = available_ranges[i];
            if(range.table_size >= num_descriptors && range.table_size - num_descriptors < waste_in_best_match) {
                best_match_idx = i;
                waste_in_best_match = range.table_size - num_descriptors;
            }
        }

        if(best_match_idx != available_ranges.size()) {
            const auto range = available_ranges[best_match_idx];
            available_ranges.erase(best_match_idx, best_match_idx + 1);

            return range;
        }

        const auto cpu_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE{heap->GetCPUDescriptorHandleForHeapStart(),
                                                              next_free_descriptor,
                                                              descriptor_size};
        const auto gpu_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE{heap->GetGPUDescriptorHandleForHeapStart(),
                                                              next_free_descriptor,
                                                              descriptor_size};

        next_free_descriptor += num_descriptors;

        return {cpu_handle, gpu_handle, num_descriptors};
    }

    void DescriptorAllocator::free_descriptor_range(const DescriptorRange handle) { available_ranges.push_back(handle); }

    UINT DescriptorAllocator::get_descriptor_size() const { return descriptor_size; }

    ID3D12DescriptorHeap* DescriptorAllocator::get_heap() const { return heap.Get(); }
} // namespace sanity::engine::renderer

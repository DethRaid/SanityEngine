#pragma once

#include <d3d12.h>
#include <rx/core/optional.h>
#include <rx/core/string.h>
#include <wrl/client.h>

#include "core/types.hpp"

using Microsoft::WRL::ComPtr;

namespace renderer {
    namespace guids {
        void init();
    }

    void set_object_name(ID3D12Object* object, const Rx::String& name);

    template <typename ObjectType>
    void set_object_name(ComPtr<ObjectType> object, const Rx::String& name) {
        set_object_name(object.Get(), name);
    }

    void set_command_allocator(ID3D12CommandList* commands, ID3D12CommandAllocator* allocator);

    void set_gpu_frame_idx(ID3D12Object* object, Uint32 frame_idx);

    [[nodisacrd]] Rx::String get_object_name(ID3D12Object* object);

    [[nodiscard]] ID3D12CommandAllocator* get_command_allocator(ID3D12CommandList* commands);

    [[nodiscard]] Rx::Optional<Uint32> get_gpu_frame_idx(ID3D12Object* object);
} // namespace renderer

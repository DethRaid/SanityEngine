#pragma once

#include <d3d12.h>
#include <rx/core/log.h>
#include <rx/core/optional.h>
#include <rx/core/string.h>
#include <wrl/client.h>

#include "core/types.hpp"

using Microsoft::WRL::ComPtr;

namespace renderer {
    namespace guids {
        void init();
    }

    RX_LOG("\033[32mD3D12PrivateData\033[0m", private_data_logger);

    void set_object_name(ID3D12Object* object, const Rx::String& name);

    void set_gpu_frame_idx(ID3D12Object* object, Uint32 frame_idx);

    [[nodisacrd]] Rx::String get_object_name(ID3D12Object* object);

    [[nodiscard]] Rx::Optional<Uint32> get_gpu_frame_idx(ID3D12Object* object);

    template <typename ObjectType>
    void set_object_name(ComPtr<ObjectType> object, const Rx::String& name) {
        set_object_name(object.Get(), name);
    }

    template <typename InterfaceType>
    void store_com_interface(ID3D12Object* object, InterfaceType* com_object) {
        object->SetPrivateDataInterface(__uuidof(InterfaceType), com_object);
    }

    template <typename InterfaceType>
    ComPtr<InterfaceType> get_com_interface(ID3D12Object* object) {
        UINT data_size{sizeof(InterfaceType*)};
        InterfaceType* com_interface{nullptr};
        const auto result = object->GetPrivateData(__uuidof(InterfaceType), &data_size, &com_interface);
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve COM interface from D3D12 object %s", get_object_name(object));
        }

        const auto com_ptr = ComPtr<InterfaceType>(com_interface);

        com_interface->Release();

        return com_ptr;
    }
} // namespace renderer

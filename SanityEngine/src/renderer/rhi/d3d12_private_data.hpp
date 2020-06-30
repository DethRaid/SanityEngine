#pragma once

#include <d3d12.h>
#include <rx/core/log.h>
#include <rx/core/optional.h>
#include <rx/core/string.h>
#include <winrt/base.h>

#include "core/types.hpp"

using winrt::com_ptr;

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
    void set_object_name(com_ptr<ObjectType> object, const Rx::String& name) {
        set_object_name(object.get(), name);
    }

    template <typename InterfaceType>
    void store_com_interface(ID3D12Object* object, InterfaceType* com_object) {
        object->SetPrivateDataInterface(__uuidof(InterfaceType), com_object);
    }

    template <typename InterfaceType>
    com_ptr<InterfaceType> get_com_interface(ID3D12Object* object) {
        UINT data_size{sizeof(InterfaceType*)};
        InterfaceType* com_interface{nullptr};
        const auto result = object->GetPrivateData(__uuidof(InterfaceType), &data_size, &com_interface);
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve COM interface from D3D12 object %s", get_object_name(object));
        }

        com_ptr<InterfaceType> com_pointer{};
        com_pointer.attach(com_interface);

        // Don't need to release here - `com_ptr::attach` doesn't AddRef, so the ref count is good

        return com_pointer;
    }
} // namespace renderer

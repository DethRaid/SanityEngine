#pragma once

#include <concepts>

#include <d3d12.h>

#include "core/types.hpp"
#include "renderer/rhi/helpers.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rx/core/log.h"
#include "rx/core/optional.h"
#include "rx/core/string.h"

#define PRIVATE_DATA_ATTRIBS(type) __uuidof(type), sizeof(type)

namespace sanity::engine::renderer {
    RX_LOG("\033[32mD3D12PrivateData\033[0m", private_data_logger);

    // template <typename T>
    // concept PrivateDataStore = requires(T a) {
    //     a->SetPrivateData(GUID{}, 0, nullptr);
    //     { a->GetPrivateData(GUID{}, std::convertible_to<UINT*>, static_cast<void**>(nullptr)) }
    //     ->std::convertible_to<HRESULT>;
    // };

    RX_HINT_FORCE_INLINE void set_object_name(ID3D12Object* object, const Rx::String& name) {
        const auto wide_name = name.to_utf16();

        object->SetName(reinterpret_cast<LPCWSTR>(wide_name.data()));
    }

    [[nodiscard]] RX_HINT_FORCE_INLINE Rx::String get_object_name(ID3D12Object* object) {
        UINT data_size{0};
        auto result = object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, nullptr);
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve size of object name");
            return "Unnamed object";
        }

        Rx::WideString name{};
        name.resize(data_size);
        result = object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, name.data());
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve object name");
            return "Unnamed object";
        }

        return name.to_utf8();
    }

    template <typename ObjectType>
    [[nodiscard]] RX_HINT_FORCE_INLINE ObjectType retrieve_object(ID3D12Object* d3d12_object) {
        ObjectType object{};
        auto object_size = static_cast<Uint32>(sizeof(ObjectType));
        const auto result = d3d12_object->GetPrivateData(__uuidof(ObjectType), &object_size, &object);
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve object from D3D12 object %s", d3d12_object);
            return {};
        }

        return object;
    }

    template <typename InterfaceType>
    RX_HINT_FORCE_INLINE void store_com_interface(ID3D12Object* object, InterfaceType* com_object) {
        object->SetPrivateDataInterface(__uuidof(InterfaceType), com_object);
    }

    template <typename InterfaceType>
    RX_HINT_FORCE_INLINE ComPtr<InterfaceType> get_com_interface(ID3D12Object* object) {
        UINT data_size{sizeof(InterfaceType*)};
        InterfaceType* com_interface{nullptr};
        const auto result = object->GetPrivateData(__uuidof(InterfaceType), &data_size, &com_interface);
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve COM interface from D3D12 object %s", object);
        }

        auto com_pointer = ComPtr<InterfaceType>{com_interface};

        // Don't need to release here - `ComPtr::attach` doesn't AddRef, so the ref count is good

        return com_pointer;
    }
} // namespace sanity::engine::renderer

namespace Rx {
    template <>
    struct FormatNormalize<ID3D12Object*> {
        char scratch[1024];
        const char* operator()(ID3D12Object* value) {
            const auto& object_name = sanity::engine::renderer::get_object_name(value);
            memcpy(scratch, object_name.data(), Algorithm::min(object_name.size(), sizeof(scratch)));
            return scratch;
        }
    };
} // namespace Rx

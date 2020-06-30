#include "d3d12_private_data.hpp"

#include <rx/core/log.h>

#include "rhi/helpers.hpp"

namespace renderer {
    namespace guids {
        static GUID gpu_frame_idx;

        void init() {
            CoCreateGuid(&gpu_frame_idx);
        }
    } // namespace guids

    void set_object_name(ID3D12Object* object, const Rx::String& name) {
        const auto wide_name = to_wide_string(name);

        object->SetName(reinterpret_cast<LPCWSTR>(wide_name.c_str()));
    }

    void set_gpu_frame_idx(ID3D12Object* object, Uint32 frame_idx) {
        object->SetPrivateData(guids::gpu_frame_idx, sizeof(Uint32), &frame_idx);
    }

    Rx::String get_object_name(ID3D12Object* object) {
        UINT data_size{sizeof(wchar_t*)};
        wchar_t* name{nullptr};
        const auto result = object->GetPrivateData(WKPDID_D3DDebugObjectName, &data_size, &name);
        if(FAILED(result)) {
            private_data_logger->error("Could not retrieve object name");
            return "Unnamed object";
        }

        return from_wide_string(name);
    }

    Rx::Optional<Uint32> get_gpu_frame_idx(ID3D12Object* object) {
        UINT data_size{sizeof(Uint32)};
        Uint32 gpu_frame_idx{0};
        const auto result = object->GetPrivateData(guids::gpu_frame_idx, &data_size, &gpu_frame_idx);
        if(FAILED(result)) {
            private_data_logger->error("Could not get the GPU frame of object %s", get_object_name(object));
            return Rx::nullopt;
        }

        return gpu_frame_idx;
    }
} // namespace renderer

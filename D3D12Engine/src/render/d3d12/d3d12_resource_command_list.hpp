#pragma once

#include "../resource_command_list.hpp"

namespace render {
    class D3D12ResourceCommandList : public virtual ResourceCommandList {
    public:
#pragma region ResourceCommandList
        ~D3D12ResourceCommandList() override;

        void copy_data_to_buffer(void* data, const Buffer& buffer) override;

        void dopy_data_to_image(void* data, const Image& image) override;
#pragma endregion
    };
} // namespace render

#pragma once

#include <array>
#include <vector>

#include <DirectXMath.h>
#include <stdint.h>

#include "../core/constants.hpp"
#include "../rhi/resources.hpp"

namespace rhi {
    class RenderDevice;
}

namespace renderer {
    struct CameraMatrices {
        DirectX::XMFLOAT4X4 view_matrix;
        DirectX::XMFLOAT4X4 projection_matrix;
    };

    /*!
     * \brief Abstraction for the camera matrix buffer
     */
    class CameraMatrixBuffer {
    public:
        CameraMatrixBuffer(rhi::RenderDevice& device_in, uint32_t num_in_flight_frames);

        CameraMatrixBuffer(const CameraMatrixBuffer& other) = delete;
        CameraMatrixBuffer& operator=(const CameraMatrixBuffer& other) = delete;

        CameraMatrixBuffer(CameraMatrixBuffer&& old) noexcept = default;
        CameraMatrixBuffer& operator=(CameraMatrixBuffer&& old) noexcept = default;

        ~CameraMatrixBuffer();

        void set_camera_matrices(uint32_t camera_idx, const CameraMatrices& matrices);

        [[nodiscard]] rhi::Buffer& get_device_buffer_for_frame(uint32_t frame_idx) const;

        [[nodiscard]] const CameraMatrices* get_host_data_pointer() const;

    private:
        rhi::RenderDevice* device;

        std::array<CameraMatrices, MAX_NUM_CAMERAS> host_data;

        std::vector<rhi::Buffer*> device_data;
    };
} // namespace renderer

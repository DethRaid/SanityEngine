#include "camera_matrix_buffer.hpp"

#include "../core/ensure.hpp"
#include "../rhi/render_device.hpp"

namespace renderer {
    CameraMatrixBuffer::CameraMatrixBuffer(rhi::RenderDevice& device_in, const uint32_t num_in_flight_frames) : device{&device_in} {
        device_data.reserve(num_in_flight_frames);

        rhi::BufferCreateInfo create_info{};
        create_info.name = "Camera Matrices";
        create_info.usage = rhi::BufferUsage::ConstantBuffer;
        create_info.size = sizeof(CameraMatrices) * MAX_NUM_CAMERAS;

        for(uint32_t i = 0; i < num_in_flight_frames; i++) {
            auto buffer = device->create_buffer(create_info);
            device_data.push_back(buffer.release());
        }
    }

    CameraMatrixBuffer::~CameraMatrixBuffer() {
        for(auto* buffer : device_data) {
            device->destroy_buffer(std::unique_ptr<rhi::Buffer>(buffer));
        }
    }

    void CameraMatrixBuffer::set_camera_matrices(const uint32_t camera_idx, const CameraMatrices& matrices) {
        ENSURE(camera_idx < MAX_NUM_CAMERAS, "Camera index {} must be less than MAX_NUM_CAMERAS ({})", camera_idx, MAX_NUM_CAMERAS);

        host_data[camera_idx] = matrices;
    }

    rhi::Buffer& CameraMatrixBuffer::get_device_buffer_for_frame(const uint32_t frame_idx) const {
        ENSURE(frame_idx < device_data.size(),
               "Not enough device buffers! There are {} device buffers for camera matrices, but buffer {} was requested",
               device_data.size(),
               frame_idx);

        return *device_data[frame_idx];
    }

    const CameraMatrices* CameraMatrixBuffer::get_host_data_pointer() const { return host_data.data(); }
} // namespace renderer

#include "camera_matrix_buffer.hpp"

#include <glm/gtx/quaternion.hpp>

#include "../core/components.hpp"
#include "../core/ensure.hpp"
#include "../rhi/helpers.hpp"
#include "../rhi/render_device.hpp"

namespace renderer {
    void CameraMatrices::copy_matrices_to_previous() {
        previous_view_matrix = view_matrix;
        previous_projection_matrix = projection_matrix;
        previous_inverse_view_matrix = inverse_view_matrix;
        previous_inverse_projection_matrix = inverse_projection_matrix;
    }

    void CameraMatrices::calculate_view_matrix(const TransformComponent& transform) {
        view_matrix = glm::mat4_cast(transform.rotation);

        view_matrix = glm::translate(view_matrix, -transform.location);

        inverse_view_matrix = glm::inverse(view_matrix);
    }

    void CameraMatrices::calculate_projection_matrix(const CameraComponent& camera) {
        ENSURE(camera.fov >= 0, "Field of view must not be negative");

        projection_matrix = {};

        if(camera.fov > 0) {
            // Use an infinite projection matrix
            // Math from http://www.terathon.com/gdc07_lengyel.pdf
            const auto e = 1.0 / tan(camera.fov / 2.0);
            projection_matrix[0][0] = static_cast<float>(e);
            projection_matrix[1][1] = static_cast<float>(e / camera.aspect_ratio);
            projection_matrix[2][2] = static_cast<float>(FLT_EPSILON - 1.0);
            projection_matrix[2][3] = -1.0f;
            projection_matrix[3][2] = static_cast<float>((FLT_EPSILON - 2) * camera.near_clip_plane);

        } else {
            const auto half_width = camera.orthographic_size / 2.0;
            const auto half_height = half_width / camera.aspect_ratio;
            projection_matrix = glm::ortho(-half_width, half_width, -half_height, half_height, 0.0, 1000.0);
        }

        inverse_projection_matrix = glm::inverse(projection_matrix);
    }

    CameraMatrixBuffer::CameraMatrixBuffer(rhi::RenderDevice& device_in, const uint32_t num_in_flight_frames) : device{&device_in} {
        device_data.reserve(num_in_flight_frames);

        auto create_info = rhi::BufferCreateInfo{.usage = rhi::BufferUsage::ConstantBuffer,
                                                 .size = sizeof(CameraMatrices) * MAX_NUM_CAMERAS};

        for(uint32_t i = 0; i < num_in_flight_frames; i++) {
            create_info.name = fmt::format("Camera Matrix Buffer {}", i).c_str();
            auto buffer = device->create_buffer(create_info);
            device_data.push_back(buffer.release());
        }
    }

    CameraMatrixBuffer::~CameraMatrixBuffer() {
        device_data.each_fwd([&](const rhi::Buffer* buffer) { device->schedule_buffer_destruction(std::unique_ptr<rhi::Buffer>{buffer}); });
    }

    CameraMatrices& CameraMatrixBuffer::get_camera_matrices(const uint32_t idx) {
        ENSURE(idx < MAX_NUM_CAMERAS, "Requested camera index {} is larger than the maximum number of cameras {}", idx, MAX_NUM_CAMERAS);

        return host_data[idx];
    }

    const CameraMatrices& CameraMatrixBuffer::get_camera_matrices(const uint32_t idx) const {
        ENSURE(idx < MAX_NUM_CAMERAS, "Requested camera index {} is larger than the maximum number of cameras {}", idx, MAX_NUM_CAMERAS);

        return host_data[idx];
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

    void CameraMatrixBuffer::upload_data(const uint32_t frame_idx) const {
        const auto num_bytes_to_upload = static_cast<uint32_t>(host_data.size()) * sizeof(CameraMatrices);

        memcpy(device_data[frame_idx]->mapped_ptr, host_data.data(), num_bytes_to_upload);
    }
} // namespace renderer

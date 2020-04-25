#include "camera_matrix_buffer.hpp"

#include "../core/components.hpp"
#include "../core/ensure.hpp"
#include "../rhi/render_device.hpp"

using namespace DirectX;

namespace renderer {
    void CameraMatrices::calculate_view_matrix(const TransformComponent& transform) {
        const auto position = XMLoadFloat4(&transform.position);
        const auto translation_matrix = XMMatrixTranslationFromVector(position);

        const auto rotation_x = XMMatrixRotationX(transform.rotation.x);
        const auto rotation_y = XMMatrixRotationY(transform.rotation.y);
        const auto rotation_z = XMMatrixRotationZ(transform.rotation.z);

        // Cameras ignore their transform's scale

        const auto view = rotation_x * rotation_y * rotation_z * translation_matrix;
        XMStoreFloat4x4(&view_matrix, view);
    }

    void CameraMatrices::calculate_projection_matrix(const CameraComponent& camera) {
        // Use an infinite projection matrix
        // Math from http://www.terathon.com/gdc07_lengyel.pdf

        const auto e = 1.0 / tan(camera.fov / 2.0);

        // Reset the projection matrix, just to be sure
        projection_matrix = {};

        // Infinite projection matrix
        projection_matrix.m[0][0] = static_cast<float>(e);
        projection_matrix.m[1][1] = static_cast<float>(e / camera.aspect_ratio);
        projection_matrix.m[2][2] = static_cast<float>(FLT_EPSILON - 1.0);
        projection_matrix.m[2][3] = -1.0f;
        projection_matrix.m[3][2] = static_cast<float>((FLT_EPSILON - 2) * camera.near_clip_plane);
    }

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

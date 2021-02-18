#include "camera_matrix_buffer.hpp"

#include "core/components.hpp"
#include "glm/gtx/quaternion.hpp"
#include "renderer.hpp"
#include "rhi/render_backend.hpp"

namespace sanity::engine::renderer {
    void CameraMatrices::copy_matrices_to_previous() {
        previous_view_matrix = view_matrix;
        previous_projection_matrix = projection_matrix;
        previous_inverse_view_matrix = inverse_view_matrix;
        previous_inverse_projection_matrix = inverse_projection_matrix;
    }

    void CameraMatrices::calculate_view_matrix(const TransformComponent& transform_component) {
        const auto& transform = transform_component.transform;
        view_matrix = mat4_cast(transform.rotation);

        view_matrix = translate(view_matrix, -static_cast<glm::vec3>(transform.location));

        inverse_view_matrix = inverse(view_matrix);
    }

    void CameraMatrices::calculate_projection_matrix(const CameraComponent& camera) {
        RX_ASSERT(camera.fov >= 0, "Field of view must not be negative");

        projection_matrix = {};

        if(camera.fov > 0) {
            projection_matrix = glm::tweakedInfinitePerspective(glm::radians(camera.fov), camera.aspect_ratio, camera.near_clip_plane);

        } else {
            const auto half_width = camera.orthographic_size / 2.0;
            const auto half_height = half_width / camera.aspect_ratio;
            projection_matrix = glm::ortho(-half_width, half_width, -half_height, half_height, 0.0, 1000.0);
        }

        inverse_projection_matrix = inverse(projection_matrix);
    }

    CameraMatrixBuffer::CameraMatrixBuffer(Renderer& renderer) : device{&renderer.get_render_backend()} {
        const auto num_gpu_frames = device->get_max_num_gpu_frames();
        device_data.reserve(num_gpu_frames);

        auto create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer, .size = sizeof(CameraMatrices) * MAX_NUM_CAMERAS};

        for(Uint32 i = 0; i < num_gpu_frames; i++) {
            create_info.name = Rx::String::format("Camera Matrix Buffer {}", i);
            auto buffer = renderer.create_buffer(create_info);
            device_data.push_back(buffer);
        }
    }

    CameraMatrixBuffer::~CameraMatrixBuffer() {
        device_data.each_fwd([&](const Buffer& buffer) { device->schedule_buffer_destruction(buffer); });
    }

    CameraMatrices& CameraMatrixBuffer::get_camera_matrices(const Uint32 idx) {
        RX_ASSERT(idx < MAX_NUM_CAMERAS, "Requested camera index %u is larger than the maximum number of cameras %u", idx, MAX_NUM_CAMERAS);

        return host_data[idx];
    }

    const CameraMatrices& CameraMatrixBuffer::get_camera_matrices(const Uint32 idx) const {
        RX_ASSERT(idx < MAX_NUM_CAMERAS, "Requested camera index %u is larger than the maximum number of cameras %u", idx, MAX_NUM_CAMERAS);

        return host_data[idx];
    }

    void CameraMatrixBuffer::set_camera_matrices(const Uint32 camera_idx, const CameraMatrices& matrices) {
        RX_ASSERT(camera_idx < MAX_NUM_CAMERAS, "Camera index %u must be less than MAX_NUM_CAMERAS (%u)", camera_idx, MAX_NUM_CAMERAS);

        host_data[camera_idx] = matrices;
    }

    void CameraMatrixBuffer::upload_data(const Uint32 frame_idx) const {

        const auto& camera_buffer_handle = get_device_buffer_for_frame(frame_idx);

        const auto& camera_data = get_host_data();
        const auto num_bytes_to_upload = static_cast<Uint32>(camera_data.size()) * sizeof(CameraMatrices);

        memcpy(camera_buffer_handle->mapped_ptr, camera_data.data(), num_bytes_to_upload);
    }

    const BufferHandle& CameraMatrixBuffer::get_device_buffer_for_frame(const Uint32 frame_idx) const {
        RX_ASSERT(frame_idx < device_data.size(),
                  "Not enough device buffers! There are %u device buffers for camera matrices, but buffer %u was requested",
                  device_data.size(),
                  frame_idx);

        return device_data[frame_idx];
    }

    const Rx::Array<CameraMatrices[MAX_NUM_CAMERAS]>& CameraMatrixBuffer::get_host_data() const { return host_data; }
} // namespace sanity::engine::renderer

#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <stdint.h>

#include "../core/components.hpp"
#include "../core/constants.hpp"
#include "../rhi/render_command_list.hpp"
#include "../rhi/resources.hpp"
#include "render_components.hpp"

namespace rhi {
    class RenderDevice;
}

namespace renderer {
    struct CameraMatrices {
        glm::mat4 view_matrix{};
        glm::mat4 projection_matrix{};
        glm::mat4 inverse_view_matrix{};
        glm::mat4 inverse_projection_matrix{};

        glm::mat4 previous_view_matrix{};
        glm::mat4 previous_projection_matrix{};
        glm::mat4 previous_inverse_view_matrix{};
        glm::mat4 previous_inverse_projection_matrix{};

        void copy_matrices_to_previous();

        void calculate_view_matrix(const TransformComponent& transform);

        void calculate_projection_matrix(const CameraComponent& camera);
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

        [[nodiscard]] CameraMatrices& get_camera_matrices(uint32_t idx);

        [[nodiscard]] const CameraMatrices& get_camera_matrices(uint32_t idx) const;

        void set_camera_matrices(uint32_t camera_idx, const CameraMatrices& matrices);

        [[nodiscard]] rhi::Buffer& get_device_buffer_for_frame(uint32_t frame_idx) const;

        [[nodiscard]] const CameraMatrices* get_host_data_pointer() const;

        /*!
         * \brief Records a command to upload the host buffer to the device buffer for the provided frame
         */
        void record_data_upload(const ComPtr<ID3D12GraphicsCommandList4>& commands, uint32_t frame_idx) const;

    private:
        rhi::RenderDevice* device;

        std::array<CameraMatrices, MAX_NUM_CAMERAS> host_data{};

        std::vector<rhi::Buffer*> device_data;
    };
} // namespace renderer

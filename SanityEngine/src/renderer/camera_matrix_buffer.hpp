#pragma once

#include "core/Prelude.hpp"
#include "core/components.hpp"
#include "core/constants.hpp"
#include "glm/glm.hpp"
#include "renderer/render_components.hpp"
#include "rhi/resources.hpp"
#include "rx/core/vector.h"

namespace sanity::engine::renderer {
    class Renderer;
    class RenderBackend;

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

        void calculate_view_matrix(const TransformComponent& transform_component);

        void calculate_projection_matrix(const CameraComponent& camera);
    };

    /*!
     * \brief Abstraction for the camera matrix buffer
     */
    class CameraMatrixBuffer {
    public:
        explicit CameraMatrixBuffer(Renderer& renderer);

        CameraMatrixBuffer(const CameraMatrixBuffer& other) = delete;
        CameraMatrixBuffer& operator=(const CameraMatrixBuffer& other) = delete;

        CameraMatrixBuffer(CameraMatrixBuffer&& old) noexcept = default;
        CameraMatrixBuffer& operator=(CameraMatrixBuffer&& old) noexcept = default;

        ~CameraMatrixBuffer();

        [[nodiscard]] CameraMatrices& get_camera_matrices(Uint32 idx);

        [[nodiscard]] const CameraMatrices& get_camera_matrices(Uint32 idx) const;

        void set_camera_matrices(Uint32 camera_idx, const CameraMatrices& matrices);

        void upload_data(Uint32 frame_idx) const;

        [[nodiscard]] const BufferHandle& get_device_buffer_for_frame(Uint32 frame_idx) const;

        [[nodiscard]] const Rx::Array<CameraMatrices[MAX_NUM_CAMERAS]>& get_host_data() const;

    private:
        RenderBackend* device;

        Rx::Array<CameraMatrices[MAX_NUM_CAMERAS]> host_data{};

        Rx::Vector<BufferHandle> device_data;
    };
} // namespace sanity::engine::renderer

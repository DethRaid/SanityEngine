#pragma once

#include <rx/core/optional.h>
#include <rx/core/vector.h>

#include "compute_command_list.hpp"
#include "resources.hpp"

namespace render {

    class MeshDataStore;

    class GraphicsCommandList : public virtual ComputeCommandList {
        /*!
         * \brief Sets the render targets that draws will render to
         */
        virtual void set_render_targets(const rx::vector<Image*>& color_targets,
                                        rx::optional<Image*> depth_target = rx::nullopt) = 0;

        /*!
         * \brief Sets the state of the graphics rendering pipeline
         */
        virtual void set_pipeline_state(const GraphicsPipelineState& state) = 0;

        /*!
         * \brief Sets the buffer to read camera matrices from
         */
        virtual void set_camera_buffer(const Buffer& camera_buffer) = 0;

        /*!
         * \brief Sets the buffer to read material data from
         */
        virtual void set_material_data_buffer(const Buffer& material_data_buffer) = 0;

        /*!
         * \brief Sets the array to read textures from
         */
        virtual void set_textures_array(const rx::vector<Image*>& textures) = 0;

        /*!
         * \brief Binds the provided mesh data to the command list. Subsequent drawcalls will render this mesh data, until you bind new mesh
         * data
         */
        virtual void bind_mesh_data(const MeshDataStore& mesh_data) = 0;

        /*!
         * \brief Draws some of the indices in the current mesh data
         *
         * This method MUST be called after bind_mesh_data
         */
        virtual void draw(uint32_t num_indices, uint32_t first_index = 0, uint32_t num_instances = 1) = 0;

        /*!
         * \brief Presents the current backbuffer to the screen
         */
        virtual void present_backbuffer() = 0;
    };
} // namespace render

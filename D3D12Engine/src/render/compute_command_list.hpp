#pragma once

#include <stdint.h>

#include "resource_command_list.hpp"

namespace render
{
    struct ComputePipelineState;
    struct BindGroup;

    /*!
     * \brief A command list which can execute compute tasks
     */
    class ComputeCommandList : virtual public ResourceCommandList
    {
    public:
        /*!
         * \brief Sets the state of the compute pipeline
         *
         * MUST be called before `bind` or `dispatch`
         */
        virtual void set_pipeline_state(const ComputePipelineState& state) = 0;

        /*!
         * \brief Binds resources for use by compute dispatches
         *
         * MUST be called after `set_pipeline_state`
         */
        virtual void bind_compute_resources(const BindGroup& resources) = 0;

        /*!
         * \brief Dispatches a compute workgroup to perform some work
         *
         * MUST be called after set_pipeline_state
         */
        virtual void dispatch(uint32_t workgroup_x, uint32_t workgroup_y = 1, uint32_t workgroup_z = 1) = 0;
    };

}

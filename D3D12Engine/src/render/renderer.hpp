#pragma once

#include <rx/core/ptr.h>

#include "compute_command_list.hpp"
#include "resource_command_list.hpp"
#include "graphics_command_list.hpp"

namespace render
{
    /*
     * \brief A device which can be used to render
     */
    class RenderDevice : rx::concepts::interface
    {
    public:
        [[nodiscard]] virtual rx::ptr<ResourceCommandList> get_resource_command_list() = 0;

        [[nodiscard]] virtual rx::ptr<ComputeCommandList> get_compute_command_list() = 0;

        [[nodiscard]] virtual rx::ptr<GraphicsCommandList> get_graphics_command_list() = 0;

        void virtual submit_command_list(rx::ptr<CommandList> commands) = 0;
    };
}

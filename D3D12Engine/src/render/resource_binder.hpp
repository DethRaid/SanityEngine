#pragma once

#include <rx/core/concepts/interface.h>

#include <rx/core/vector.h>

namespace render {
    struct Buffer;
    struct Image;

    /*!
     * \brief Abstraction you can use to bind resources to a command list
     */
    class ResourceBinder : rx::concepts::interface {};
} // namespace render

#pragma once

#include <ftl/task.h>
#include <rx/core/optional.h>
#include <rx/core/string.h>
#include <rx/core/vector.h>

#include "renderer/handles.hpp"

namespace renderer {
    class Renderer;
}

/*!
 * \brief Loads an image from disk
 *
 * Currently only supports RGB and RGBA images
 */
bool load_image(const Rx::String& image_name, Uint32& width, Uint32& height, Rx::Vector<uint8_t>& pixels);

Rx::Optional<renderer::TextureHandle> load_image_to_gpu(const Rx::String& texture_name, renderer::Renderer& renderer);

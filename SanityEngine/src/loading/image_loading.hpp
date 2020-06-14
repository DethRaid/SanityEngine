#pragma once

#include <ftl/task.h>
#include <rx/core/optional.h>
#include <rx/core/string.h>
#include <rx/core/vector.h>

#include "renderer/handles.hpp"

namespace renderer {
    class Renderer;
}

struct LoadImageToGpuArgs {
    Rx::String texture_name_in;
    Rx::Optional<renderer::TextureHandle> handle_out;
    renderer::Renderer* renderer_in;
};

/*!
 * \brief Loads an image from disk
 *
 * Currently only supports RGB and RGBA images
 */
bool load_image(const Rx::String& image_name, uint32_t& width, uint32_t& height, Rx::Vector<uint8_t>& pixels);

FTL_TASK_ENTRY_POINT(load_image_to_gpu);

#pragma once
#include <optional>
#include <string>
#include <vector>
#include <glm/vec4.hpp>

#include "../renderer/handles.hpp"

namespace renderer {
    class Renderer;
}

namespace ftl {
    class TaskScheduler;
}

struct LoadImageToGpuData {
    std::string texture_name;
    std::optional<renderer::TextureHandle> handle;
    renderer::Renderer* renderer;
};

/*!
 * \brief Loads an image from disk
 *
 * Currently only supports RGB and RGBA images
 */
bool load_image(const std::string& image_name, uint32_t& width, uint32_t& height, std::vector<uint8_t>& pixels);

void load_image_to_gpu(ftl::TaskScheduler* scheduler, void* data);

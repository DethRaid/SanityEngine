#pragma once
#include <string>
#include <vector>
#include <glm/vec4.hpp>

/*!
 * \brief Loads an image from disk
 *
 * Currently only supports RGB and RGBA images
 */
bool load_image(const std::string& image_name, uint32_t& width, uint32_t& height, std::vector<uint8_t>& pixels);

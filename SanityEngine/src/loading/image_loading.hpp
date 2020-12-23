#pragma once

#include "renderer/handles.hpp"
#include "rx/core/optional.h"
#include "rx/core/string.h"
#include "rx/core/vector.h"

namespace std {
	namespace filesystem {
		class path;
	}
}

namespace sanity::engine {
    namespace renderer {
        class Renderer;
    }

    /*!
     * \brief Loads an image from disk
     *
     * Currently only supports RGB and RGBA images
     */
    bool load_image(const std::filesystem::path& image_name, Uint32& width, Uint32& height, Rx::Vector<Uint8>& pixels);

    Rx::Optional<renderer::TextureHandle> load_image_to_gpu(const std::filesystem::path& texture_name, renderer::Renderer& renderer);
} // namespace sanity::engine

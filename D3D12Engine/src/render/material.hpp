#pragma once

#ifdef interface
#undef interface
#endif

#include <rx/core/concepts/interface.h>
#include <rx/core/ptr.h>
#include <rx/core/string.h>
#include <rx/core/vector.h>

namespace render {
    struct Buffer;
    struct Image;

    struct Material {};

    class MaterialBuilder : rx::concepts::interface {
    public:
        virtual MaterialBuilder& set_buffer(const rx::string& name, const Buffer& buffer) = 0;

        virtual MaterialBuilder& set_image(const rx::string& name, const Image& image) = 0;

        virtual MaterialBuilder& set_image_array(const rx::string& name, const rx::vector<const Image*>& images) = 0;

        virtual rx::ptr<Material> build() = 0;
    };
} // namespace render

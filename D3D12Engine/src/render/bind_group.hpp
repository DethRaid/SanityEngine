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

    struct BindGroup {};

    class BindGroupBuilder : rx::concepts::interface {
    public:
        virtual BindGroupBuilder& set_buffer(const std::string& name, const Buffer& buffer) = 0;

        virtual BindGroupBuilder& set_image(const std::string& name, const Image& image) = 0;

        virtual BindGroupBuilder& set_image_array(const std::string& name, const std::vector<const Image*>& images) = 0;

        virtual std::unique_ptr<BindGroup> build() = 0;
    };
} // namespace render

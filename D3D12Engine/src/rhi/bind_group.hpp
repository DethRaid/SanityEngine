#pragma once

#include <memory>
#include <string>
#include <vector>

namespace rhi {
    struct Buffer;
    struct Image;

    struct BindGroup {};

    class BindGroupBuilder {
    public:
        virtual ~BindGroupBuilder() = default;

        virtual BindGroupBuilder& set_buffer(const std::string& name, const Buffer& buffer) = 0;

        virtual BindGroupBuilder& set_image(const std::string& name, const Image& image) = 0;

        virtual BindGroupBuilder& set_image_array(const std::string& name, const std::vector<const Image*>& images) = 0;

        virtual std::unique_ptr<BindGroup> build() = 0;
    };
} // namespace render

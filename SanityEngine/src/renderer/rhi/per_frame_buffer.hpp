#pragma once

#include "core/types.hpp"
#include "renderer/rhi/resources.hpp"

namespace sanity::engine::renderer {
    class PerFrameBuffer {
    public:
        PerFrameBuffer() = default;

        explicit PerFrameBuffer(Rx::Vector<BufferHandle> buffers_in);

        void set_buffers(Rx::Vector<BufferHandle> buffers_in);

        void advance_frame();

        template <typename DataType>
        void upload_data_to_active_buffer(const Rx::Vector<DataType>& data);

        [[nodiscard]] BufferHandle get_active_buffer() const;

    private:
        Rx::Vector<BufferHandle> buffers;
        Uint32 active_buffer_idx{0};
    };

    template <typename DataType>
    void PerFrameBuffer::upload_data_to_active_buffer(const Rx::Vector<DataType>& data) {
        const auto& active_buffer = get_active_buffer();

    	memcpy(active_buffer->mapped_ptr, data.data(), data.size() * sizeof(DataType));
    }
} // namespace sanity::engine::renderer

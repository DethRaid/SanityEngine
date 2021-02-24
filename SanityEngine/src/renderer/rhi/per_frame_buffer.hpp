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

        [[nodiscard]] BufferHandle get_active_buffer() const;

    private:
        Rx::Vector<BufferHandle> buffers;
        Uint32 active_buffer_idx{0};
    };
} // namespace sanity::engine::renderer

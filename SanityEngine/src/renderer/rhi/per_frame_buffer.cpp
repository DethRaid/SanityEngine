#include "per_frame_buffer.hpp"

namespace sanity::engine::renderer {
    PerFrameBuffer::PerFrameBuffer(Rx::Vector<BufferHandle> buffers_in) : buffers{Rx::Utility::move(buffers_in)} {}

    void PerFrameBuffer::set_buffers(Rx::Vector<BufferHandle> buffers_in) { buffers = Rx::Utility::move(buffers_in); }

    void PerFrameBuffer::advance_frame() {
        active_buffer_idx++;
        if(active_buffer_idx >= buffers.size()) {
            active_buffer_idx = 0;
        }
    }

    BufferHandle PerFrameBuffer::get_active_buffer() const { return buffers[active_buffer_idx]; }
} // namespace sanity::engine::renderer

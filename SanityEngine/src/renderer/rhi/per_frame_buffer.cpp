#include "per_frame_buffer.hpp"

#include "renderer/renderer.hpp"

namespace sanity::engine::renderer {
    PerFrameBuffer::PerFrameBuffer(const Rx::String& name, Uint32 size, Renderer& renderer) {
	    const auto num_frames = renderer.get_render_backend().get_max_num_gpu_frames();
    	buffers.reserve(num_frames);

    	for(auto i = 0u; i < num_frames; i++) {
    		const auto create_info = BufferCreateInfo {
    			.name = Rx::String::format("%s buffer %d", name, i),
    			.usage = BufferUsage::ConstantBuffer,
    			.size = size
    		};
    		const auto handle = renderer.create_buffer(create_info);

    		buffers.push_back(handle);
    	}
    }

    void PerFrameBuffer::advance_frame() {
        active_buffer_idx++;
        if(active_buffer_idx >= buffers.size()) {
            active_buffer_idx = 0;
        }
    }

    BufferHandle PerFrameBuffer::get_active_buffer() const { return buffers[active_buffer_idx]; }
} // namespace sanity::engine::renderer

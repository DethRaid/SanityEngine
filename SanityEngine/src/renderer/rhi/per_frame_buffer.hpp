#pragma once

#include "core/types.hpp"
#include "renderer/rhi/resources.hpp"

namespace sanity::engine::renderer {
	class Renderer;

	class PerFrameBuffer {
    public:
        PerFrameBuffer() = default;
		
        explicit PerFrameBuffer(const Rx::String& name, Uint32 size, Renderer& renderer);

        void advance_frame();

        [[nodiscard]] BufferHandle get_active_buffer() const;

    private:
        Rx::Vector<BufferHandle> buffers;
        Uint32 active_buffer_idx{0};
    };
} // namespace sanity::engine::renderer

#include "material_data_buffer.hpp"

namespace renderer {
    MaterialDataBuffer::MaterialDataBuffer(const uint32_t buffer_size_in)
        : buffer{new uint8_t[buffer_size_in]}, buffer_size{buffer_size_in} {}

    uint8_t* MaterialDataBuffer::data() const { return buffer; }

    uint32_t MaterialDataBuffer::size() const { return buffer_size; }
} // namespace renderer

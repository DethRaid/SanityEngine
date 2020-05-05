#include "material_data_buffer.hpp"

namespace renderer {
    MaterialDataBuffer::MaterialDataBuffer(const size_t buffer_size_in) : buffer{new uint8_t[buffer_size_in]} {}

    uint8_t* MaterialDataBuffer::data() const { return buffer; }
} // namespace renderer

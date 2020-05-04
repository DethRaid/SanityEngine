#include "material_data_buffer.hpp"


namespace nova::renderer {
    MaterialDataBuffer::MaterialDataBuffer(uint8_t* buffer_in, const size_t buffer_size_in) : buffer{buffer_in}, buffer_size{buffer_size_in} {}

    uint8_t* MaterialDataBuffer::data() const { return buffer; }
} // namespace nova::renderer

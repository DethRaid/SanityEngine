#include "material_data_buffer.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace renderer {
    std::shared_ptr<spdlog::logger> MaterialDataBuffer::logger = spdlog::stdout_color_st("MaterialDataBuffer");

    MaterialDataBuffer::MaterialDataBuffer(const uint32_t buffer_size_in)
        : buffer{new uint8_t[buffer_size_in]}, buffer_size{buffer_size_in} {
        logger->set_level(spdlog::level::trace);
    }

    uint8_t* MaterialDataBuffer::data() const { return buffer; }

    uint32_t MaterialDataBuffer::size() const { return buffer_size; }
} // namespace renderer

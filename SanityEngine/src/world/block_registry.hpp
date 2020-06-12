#pragma once

#include <cstdint>

struct BlockId {
    uint32_t id{0};
};

class BlockRegistry {
public:
    static constexpr BlockId AIR{0};
    static constexpr BlockId DIRT{1};
    static constexpr BlockId STONE{2};
};

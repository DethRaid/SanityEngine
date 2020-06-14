#pragma once

#include <cstdint>

struct BlockId {
    Uint32 id{0};
};

class BlockRegistry {
public:
    static constexpr BlockId AIR{0};
    static constexpr BlockId DIRT{1};
    static constexpr BlockId STONE{2};

    // TODO: A good way to register blocks, and block materials
    // I'd like a data format on disk? But I'm not sure how data-driven I want to be just yet
};

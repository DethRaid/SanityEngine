#pragma once
#include <vector>

namespace renderer {
    class Texture2D;
}

class World {
public:
    static World create(const renderer::Texture2D& noise_texture);

private:
    explicit World(const std::vector<std::vector<float>>& terrain_heightmap);
};

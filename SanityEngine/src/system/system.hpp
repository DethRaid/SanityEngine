#pragma once

namespace sanity::engine {
    class System {
    public:
        virtual ~System() = default;

        virtual void tick(float delta_time) = 0;
    };
} // namespace sanity::engine

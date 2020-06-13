#pragma once

#include <functional>

#include <rx/core/vector.h>

class InputManager {
public:
    void on_key(int key, int action, int mods) const;

    void register_key_callback(std::function<void(int, int, int)>&& callback);

private:
    Rx::Vector<std::function<void(int, int, int)>> key_callbacks;
};

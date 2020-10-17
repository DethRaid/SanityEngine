#pragma once

#include "core/Prelude.hpp"
#include "rx/core/function.h"
#include "rx/core/vector.h"

class InputManager {
public:
    void on_key(int key, int action, int mods) const;

    void register_key_callback(Rx::Function<void(int, int, int)>&& callback);

private:
    Rx::Vector<Rx::Function<void(int, int, int)>> key_callbacks;
};

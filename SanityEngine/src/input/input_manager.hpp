#pragma once

#include "core/Prelude.hpp"
#include "rx/core/function.h"
#include "rx/core/vector.h"

class InputManager {
public:
    void on_key(int key, int action, int mods) const;

	void on_mouse_button(int button, int action, int mods) const;

    void register_key_callback(Rx::Function<void(int, int, int)>&& callback);

	void register_mouse_button_callback(Rx::Function<void(int, int, int)>&& callback);

private:
    Rx::Vector<Rx::Function<void(int, int, int)>> key_callbacks;

    Rx::Vector<Rx::Function<void(int, int, int)>> mouse_button_callbacks;
};

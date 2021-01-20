#include "input_manager.hpp"

void InputManager::on_key(const int key, const int action, const int mods) const {
    key_callbacks.each_fwd([&](const Rx::Function<void(int, int, int)>& callback) { callback(key, action, mods); });
}

void InputManager::on_mouse_button(const int button, const int action, const int mods) const {
    mouse_button_callbacks.each_fwd([&](const Rx::Function<void(int, int, int)>& callback) { callback(button, action, mods); });
}

void InputManager::register_key_callback(Rx::Function<void(int, int, int)>&& callback) {
    key_callbacks.push_back(Rx::Utility::move(callback));
}

void InputManager::register_mouse_button_callback(Rx::Function<void(int, int, int)>&& callback) {
    mouse_button_callbacks.push_back(Rx::Utility::move(callback));
}

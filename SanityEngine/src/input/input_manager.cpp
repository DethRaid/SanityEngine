#include "input_manager.hpp"

void InputManager::on_key(const int key, const int action, const int mods) const {
    for(const auto& callback : key_callbacks) {
        callback(key, action, mods);
    }
}

void InputManager::register_key_callback(std::function<void(int, int, int)>&& callback) {
    key_callbacks.push_back(std::move(callback));
}

#include "input_manager.hpp"

void InputManager::on_key(const int key, const int action, const int mods) const {
    key_callbacks.each_fwd([&](const std::function<void(int, int, int)>& callback) { callback(key, action, mods); });
}

void InputManager::register_key_callback(std::function<void(int, int, int)>&& callback) { key_callbacks.push_back(Rx::Utility::move(callback)); }

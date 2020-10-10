#include "input_manager.hpp"

InputManager::InputManager()
{
	// This only exists so I can breakpoint it
}

void InputManager::on_key(const int key, const int action, const int mods) const {
    key_callbacks.each_fwd([&](const Rx::Function<void(int, int, int)>& callback) { callback(key, action, mods); });
}

void InputManager::register_key_callback(Rx::Function<void(int, int, int)>&& callback) { key_callbacks.push_back(Rx::Utility::move(callback)); }

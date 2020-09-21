#pragma once
#include "core/types.hpp"

/// <summary>
/// Interface for SanityEngine to access the input device(s) of whatever computer it's running on
/// </summary>
class PlatformInput {
public:
	virtual ~PlatformInput() = default;

	/// <summary>
	/// Returns true if the specified key was pressed down this frame, false otherwise
	/// </summary>
	[[nodiscard]] virtual bool is_key_down(int key) = 0;

	/// <summary>
	/// Returns the current location of the mouse, in window-relative pixel coordinates
	/// </summary>
	[[nodiscard]] virtual Double2 get_mouse_location() = 0;
};

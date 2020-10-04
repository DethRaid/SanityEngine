#pragma once
#include "core/types.hpp"

/// <summary>
/// All the input keys that SanityEngine cares about
/// </summary>
enum class InputKey {
	W,
	A,
	S,
	D,

	Q,
	E,

	SPACE,
	SHIFT,
};

/// <summary>
/// Interface for SanityEngine to access the input device(s) of whatever computer it's running on
/// </summary>
class PlatformInput {
public:
	virtual ~PlatformInput() = default;

	/// <summary>
	/// Returns true if the specified key was pressed down this frame, false otherwise
	/// </summary>
	[[nodiscard]] virtual bool is_key_down(InputKey key) const = 0;

	/// <summary>
	/// Returns the current location of the mouse, in window-relative pixel coordinates
	/// </summary>
	[[nodiscard]] virtual Double2 get_mouse_location() const = 0;
};

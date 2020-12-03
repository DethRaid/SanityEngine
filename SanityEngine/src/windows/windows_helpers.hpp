#pragma once

#include <Windows.h>

#include "rx/core/string.h"

Rx::String get_last_windows_error();

[[nodiscard]] Rx::String to_string(HRESULT hr);

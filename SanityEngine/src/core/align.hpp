#pragma once

#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

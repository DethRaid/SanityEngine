#ifndef RX_CORE_HINTS_FORMAT_H
#define RX_CORE_HINTS_FORMAT_H
#include "rx/core/config.h" // RX_COMPILER_{GCC, CLANG}

#if defined(RX_COMPILER_GCC) || defined(RX_COMPILER_CLANG)
#define RX_HINT_FORMAT(_format_index, _list_index) \
  __attribute__((format(printf, _format_index, _list_index)))
#else
#define RX_HINT_FORMAT(...)
#endif

#endif // RX_CORE_HINTS_FORMAT_H

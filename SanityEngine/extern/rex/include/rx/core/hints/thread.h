#ifndef RX_CORE_HINTS_THREAD_H
#define RX_CORE_HINTS_THREAD_H
#include "rx/core/config.h" // RX_COMPILER_*

#if defined(RX_COMPILER_CLANG)
#define RX_HINT_THREAD_ATTRIBUTE(...) \
  __attribute__((__VA_ARGS__))
#else
#define RX_HINT_THREAD_ATTRIBUTE(...)
#endif

#define RX_HINT_GUARDED_BY(_lock) \
  RX_HINT_THREAD_ATTRIBUTE(guarded_by(_lock))

#define RX_HINT_ACQUIRE(...) \
  RX_HINT_THREAD_ATTRIBUTE(exclusive_lock_function(__VA_ARGS__))

#define RX_HINT_RELEASE(...) \
  RX_HINT_THREAD_ATTRIBUTE(unlock_function(__VA_ARGS__))

#define RX_HINT_LOCKABLE \
  RX_HINT_THREAD_ATTRIBUTE(lockable)

#endif // RX_CORE_HINTS_THREAD_H

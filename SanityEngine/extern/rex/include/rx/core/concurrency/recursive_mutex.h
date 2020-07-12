#ifndef RX_CORE_CONCURRENCY_RECURSIVE_MUTEX_H
#define RX_CORE_CONCURRENCY_RECURSIVE_MUTEX_H
#include "rx/core/types.h" // Byte
#include "rx/core/hints/thread.h"

namespace Rx::Concurrency {

struct RX_HINT_LOCKABLE RecursiveMutex {
  RecursiveMutex();
  ~RecursiveMutex();

  void lock() RX_HINT_ACQUIRE();
  void unlock() RX_HINT_RELEASE();

private:
  friend struct ConditionVariable;

  // Fixed-capacity storage for any OS mutex type, adjust if necessary.
  alignas(16) Byte m_mutex[64];
};

} // namespace Rx::Concurrency

#endif // RX_CORE_CONCURRENCY_RECURSIVE_MUTEX_H

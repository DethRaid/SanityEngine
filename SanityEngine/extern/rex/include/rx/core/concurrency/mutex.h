#ifndef RX_CORE_CONCURRENCY_MUTEX_H
#define RX_CORE_CONCURRENCY_MUTEX_H
#include "rx/core/types.h" // Byte
#include "rx/core/hints/thread.h"

namespace Rx::Concurrency {

struct RX_HINT_LOCKABLE Mutex {
  Mutex();
  ~Mutex();

  void lock() RX_HINT_ACQUIRE();
  void unlock() RX_HINT_RELEASE();

private:
  friend struct ConditionVariable;

  // Fixed-capacity storage for any OS mutex Type, adjust if necessary.
  alignas(16) Byte m_mutex[64];
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_MUTEX_H

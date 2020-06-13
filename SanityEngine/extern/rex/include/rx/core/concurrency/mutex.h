#ifndef RX_CORE_CONCURRENCY_MUTEX_H
#define RX_CORE_CONCURRENCY_MUTEX_H
#include "rx/core/types.h" // rx_byte

namespace Rx::Concurrency {

struct Mutex {
  Mutex();
  ~Mutex();

  void lock();
  void unlock();

private:
  friend struct ConditionVariable;

  // Fixed-capacity storage for any OS mutex Type, adjust if necessary.
  alignas(16) Byte m_mutex[64];
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_MUTEX_H

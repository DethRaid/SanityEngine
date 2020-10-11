#ifndef RX_CORE_CONCURRENCY_SPIN_LOCK_H
#define RX_CORE_CONCURRENCY_SPIN_LOCK_H
#include "rx/core/concurrency/atomic.h" // atomic_flag
#include "rx/core/hints/thread.h"

namespace Rx::Concurrency {

struct RX_API RX_HINT_LOCKABLE SpinLock {
  constexpr SpinLock();
  ~SpinLock() = default;
  void lock() RX_HINT_ACQUIRE();
  void unlock() RX_HINT_RELEASE();
private:
  AtomicFlag m_lock;
};

inline constexpr SpinLock::SpinLock()
  : m_lock{false}
{
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_SPIN_LOCK_H

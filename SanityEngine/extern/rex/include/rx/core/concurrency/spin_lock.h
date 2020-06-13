#ifndef RX_CORE_CONCURRENCY_SPIN_LOCK_H
#define RX_CORE_CONCURRENCY_SPIN_LOCK_H
#include "rx/core/concurrency/atomic.h" // atomic_flag

namespace Rx::Concurrency {

struct SpinLock {
  constexpr SpinLock();
  ~SpinLock() = default;
  void lock();
  void unlock();
private:
  AtomicFlag m_lock;
};

inline constexpr SpinLock::SpinLock()
  : m_lock{false}
{
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_SPIN_LOCK_H

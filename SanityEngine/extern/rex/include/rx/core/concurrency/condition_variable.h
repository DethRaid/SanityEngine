#ifndef RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
#define RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/concurrency/mutex.h"

namespace Rx::Concurrency {

struct ConditionVariable {
  ConditionVariable();
  ~ConditionVariable();

  void wait(Mutex& _mutex);
  void wait(ScopeLock<Mutex>& _scope_lock);

  template<typename P>
  void wait(Mutex& _mutex, P&& _predicate);

  template<typename P>
  void wait(ScopeLock<Mutex>& _scope_lock, P&& _predicate);

  void signal();
  void broadcast();

private:
  // Fixed-capacity storage for any OS condition variable Type, adjust if necessary.
  alignas(16) Byte m_cond[64];
};

inline void ConditionVariable::wait(ScopeLock<Mutex>& _scope_lock) {
  wait(_scope_lock.m_lock);
}

template<typename P>
inline void ConditionVariable::wait(Mutex& _mutex, P&& _predicate) {
  while (!_predicate()) {
    wait(_mutex);
  }
}

template<typename P>
inline void ConditionVariable::wait(ScopeLock<Mutex>& _scope_lock, P&& _predicate) {
  while (!_predicate()) {
    wait(_scope_lock);
  }
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_CONDITION_VARIABLE_H

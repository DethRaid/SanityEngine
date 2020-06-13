#ifndef RX_CORE_CONCURRENCY_SCOPE_LOCK_H
#define RX_CORE_CONCURRENCY_SCOPE_LOCK_H

namespace Rx::Concurrency {

// generic scoped lock
template<typename T>
struct ScopeLock {
  explicit constexpr ScopeLock(T& lock_);
  ~ScopeLock();
private:
  friend struct ConditionVariable;
  T& m_lock;
};

template<typename T>
inline constexpr ScopeLock<T>::ScopeLock(T& lock_)
  : m_lock{lock_}
{
  m_lock.lock();
}

template<typename T>
inline ScopeLock<T>::~ScopeLock() {
  m_lock.unlock();
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_SCOPE_LOCK_H

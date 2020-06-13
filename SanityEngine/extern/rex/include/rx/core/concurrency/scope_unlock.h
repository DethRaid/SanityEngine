#ifndef RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H
#define RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H

namespace rx::concurrency {

// generic scoped unlock
template<typename T>
struct ScopeUnlock {
  explicit constexpr ScopeUnlock(T& lock_);
  ~ScopeUnlock();
private:
  T& m_lock;
};

template<typename T>
inline constexpr ScopeUnlock<T>::ScopeUnlock(T& lock_)
  : m_lock{lock_}
{
  m_lock.unlock();
}

template<typename T>
inline ScopeUnlock<T>::~ScopeUnlock() {
  m_lock.lock();
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_SCOPE_UNLOCK_H

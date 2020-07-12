#ifndef RX_CORE_CONCURRENCY_ATOMIC_H
#define RX_CORE_CONCURRENCY_ATOMIC_H
#include "rx/core/config.h" // RX_COMPILER_*
#include "rx/core/types.h"
#include "rx/core/markers.h"

#include "rx/core/traits/is_integral.h"
#include "rx/core/traits/is_same.h"

namespace Rx::Concurrency {

enum class MemoryOrder {
  k_relaxed,
  k_consume, // load-consume
  k_acquire, // load-acquire
  k_release, // store-release
  k_acq_rel, // store-release load-acquire
  k_seq_cst  // store-release load-acquire
};

} // namespace rx::concurrency

#include "rx/core/concurrency/std/atomic.h"

#if defined(RX_COMPILER_GCC)
#include "rx/core/concurrency/gcc/atomic.h"
#elif defined(RX_COMPILER_CLANG)
#include "rx/core/concurrency/clang/atomic.h"
#else
// Use <atomic> as fallback,
#include "rx/core/concurrency/std/atomic.h"
#endif

namespace Rx::Concurrency {

namespace detail {
  template<typename T>
  struct AtomicValue : AtomicBase<T> {
    AtomicValue() = default;
    constexpr explicit AtomicValue(T _value) : AtomicBase<T>{_value} {}
  };

  template<typename T, bool = traits::is_integral<T> && !traits::is_same<T, bool>>
  struct Atomic {
    RX_MARK_NO_COPY(Atomic);

    Atomic() = default;
    constexpr Atomic(T _value) : m_value{_value} {}

    void store(T _value, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      atomic_store(&m_value, _value, _order);
    }

    void store(T _value, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      atomic_store(&m_value, _value, _order);
    }

    T load(MemoryOrder _order = MemoryOrder::k_seq_cst) const volatile {
      return atomic_load(&m_value, _order);
    }

    T load(MemoryOrder _order = MemoryOrder::k_seq_cst) const {
      return atomic_load(&m_value, _order);
    }

    operator T() const volatile {
      return load();
    }

    operator T() const {
      return load();
    }

    T exchange(T _value, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_exchange(&m_value, _value, _order);
    }

    T exchange(T _value, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_exchange(&m_value, _value, _order);
    }

    bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _success,
      MemoryOrder _failure) volatile
    {
      return atomic_compare_exchange_weak(&m_value, expected_, _value, _success, _failure);
    }

    bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _success,
      MemoryOrder _failure)
    {
      return atomic_compare_exchange_weak(&m_value, expected_, _value, _success, _failure);
    }

    bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _success,
      MemoryOrder _failure) volatile
    {
      return atomic_compare_exchange_strong(&m_value, expected_, _value, _success, _failure);
    }

    bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _success,
      MemoryOrder _failure)
    {
      return atomic_compare_exchange_strong(&m_value, expected_, _value, _success, _failure);
    }

    bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_compare_exchange_weak(&m_value, expected_, _value, _order, _order);
    }

    bool compare_exchange_weak(T& expected_, T _value, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_compare_exchange_weak(&m_value, expected_, _value, _order, _order);
    }

    bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _order) volatile {
      return atomic_compare_exchange_strong(&m_value, expected_, _value, _order, _order);
    }

    bool compare_exchange_strong(T& expected_, T _value, MemoryOrder _order) {
      return atomic_compare_exchange_strong(&m_value, expected_, _value, _order, _order);
    }

  protected:
    mutable AtomicValue<T> m_value;
  };

  // specialization for integral
  template<typename T>
  struct Atomic<T, true> : Atomic<T, false> {
    using Base = Atomic<T, false>;

    Atomic() = default;
    constexpr Atomic(T _value) : Base{_value} {}

    T fetch_add(T _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_fetch_add(&this->m_value, _delta, _order);
    }

    T fetch_add(T _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_fetch_add(&this->m_value, _delta, _order);
    }

    T fetch_sub(T _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_fetch_sub(&this->m_value, _delta, _order);
    }

    T fetch_sub(T _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_fetch_sub(&this->m_value, _delta, _order);
    }

    T fetch_and(T _pattern, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_fetch_and(&this->m_value, _pattern, _order);
    }

    T fetch_and(T _pattern, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_fetch_sub(&this->m_value, _pattern, _order);
    }

    T fetch_or(T _pattern, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_fetch_or(&this->m_value, _pattern, _order);
    }

    T fetch_or(T _pattern, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_fetch_or(&this->m_value, _pattern, _order);
    }

    T fetch_xor(T _pattern, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
      return atomic_fetch_xor(&this->m_value, _pattern, _order);
    }

    T fetch_xor(T _pattern, MemoryOrder _order = MemoryOrder::k_seq_cst) {
      return atomic_fetch_xor(&this->m_value, _pattern, _order);
    }

    // ++, --
    T operator++(int) volatile {
      return fetch_add(T{1});
    }

    T operator++(int) {
      return fetch_add(T{1});
    }

    T operator--(int) volatile {
      return fetch_sub(T{1});
    }

    T operator--(int) {
      return fetch_sub(T{1});
    }

    T operator++() volatile {
      return fetch_add(T{1}) + T{1};
    }

    T operator++() {
      return fetch_add(T{1}) + T{1};
    }

    T operator--() volatile {
      return fetch_sub(T{1}) - T{1};
    }

    T operator--() {
      return fetch_sub(T{1}) - T{1};
    }

    T operator+=(T _delta) volatile {
      return fetch_add(_delta) + _delta;
    }

    T operator+=(T _delta) {
      return fetch_add(_delta) + _delta;
    }

    T operator-=(T _delta) volatile {
      return fetch_sub(_delta) - _delta;
    }

    T operator-=(T _delta) {
      return fetch_sub(_delta) - _delta;
    }

    T operator&=(T _pattern) volatile {
      return fetch_and(_pattern) & _pattern;
    }

    T operator&=(T _pattern) {
      return fetch_and(_pattern) & _pattern;
    }

    T operator|=(T _pattern) volatile {
      return fetch_or(_pattern) | _pattern;
    }

    T operator|=(T _pattern) {
      return fetch_or(_pattern) | _pattern;
    }

    T operator^=(T _pattern) volatile {
      return fetch_xor(_pattern) ^ _pattern;
    }

    T operator^=(T _pattern) {
      return fetch_xor(_pattern) ^ _pattern;
    }
  };

} // namespace detail

template<typename T>
struct Atomic : detail::Atomic<T> {
  using Base = detail::Atomic<T>;

  Atomic() = default;
  constexpr Atomic(T _value) : Base{_value} {}

  T operator=(T _value) volatile {
    Base::store(_value);
    return _value;
  }

  T operator=(T _value) {
    Base::store(_value);
    return _value;
  }
};

template<typename T>
struct Atomic<T*> : detail::Atomic<T*> {
  using Base = detail::Atomic<T*>;

  Atomic() = default;
  constexpr Atomic(T* _value) : Base{_value} {}

  T* operator=(T* _value) volatile {
    Base::store(_value);
    return _value;
  }

  T* operator=(T* _value) {
    Base::store(_value);
    return _value;
  }

  T* fetch_add(PtrDiff _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
    return detail::atomic_fetch_add(&this->m_value, _delta, _order);
  }

  T* fetch_add(PtrDiff _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) {
    return detail::atomic_fetch_add(&this->m_value, _delta, _order);
  }

  T* fetch_sub(PtrDiff _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
    return detail::atomic_fetch_sub(&this->m_value, _delta, _order);
  }

  T* fetch_sub(PtrDiff _delta, MemoryOrder _order = MemoryOrder::k_seq_cst) {
    return detail::atomic_fetch_sub(&this->m_value, _delta, _order);
  }

  T* operator++(int) volatile {
    return fetch_add(1);
  }

  T* operator++(int) {
    return fetch_add(1);
  }

  T* operator--(int) volatile {
    return fetch_sub(1);
  }

  T* operator--(int) {
    return fetch_sub(1);
  }

  T* operator++() volatile {
    return fetch_add(1) + 1;
  }

  T* operator++() {
    return fetch_add(1) + 1;
  }

  T* operator--() volatile {
    return fetch_sub(1) - 1;
  }

  T* operator--() {
    return fetch_sub(1) - 1;
  }

  T* operator+=(PtrDiff _delta) volatile {
    return fetch_add(_delta) + _delta;
  }

  T* operator+=(PtrDiff _delta) {
    return fetch_add(_delta) + _delta;
  }

  T* operator-=(PtrDiff _delta) volatile {
    return fetch_sub(_delta) - _delta;
  }

  T* operator-=(PtrDiff _delta) {
    return fetch_sub(_delta) - _delta;
  }
};

struct AtomicFlag {
  RX_MARK_NO_COPY(AtomicFlag);

  AtomicFlag() = default;
  constexpr AtomicFlag(bool _value) : m_value{_value} {}

  bool test_and_set(MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
    return detail::atomic_exchange(&m_value, true, _order);
  }

  bool test_and_set(MemoryOrder _order = MemoryOrder::k_seq_cst)  {
    return detail::atomic_exchange(&m_value, true, _order);
  }

  void clear(MemoryOrder _order = MemoryOrder::k_seq_cst) volatile {
    detail::atomic_store(&m_value, false, _order);
  }

  void clear(MemoryOrder _order = MemoryOrder::k_seq_cst) {
    detail::atomic_store(&m_value, false, _order);
  }

private:
  detail::AtomicBase<bool> m_value;
};

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_ATOMIC_H

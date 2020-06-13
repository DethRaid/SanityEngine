#ifndef RX_CORE_CONCURRENCY_CLANG_ATOMIC_H
#define RX_CORE_CONCURRENCY_CLANG_ATOMIC_H
#if defined(RX_COMPILER_CLANG)

#include "rx/core/traits/remove_const.h"

namespace rx::concurrency::detail {

template<typename T>
struct AtomicBase {
  AtomicBase() = default;
  constexpr explicit AtomicBase(T _value) : value(_value) {}
  _Atomic(T) value;
};

inline void atomic_thread_fence(MemoryOrder _order) {
  __c11_atomic_thread_fence(static_cast<int>(_order));
}

inline void atomic_signal_fence(MemoryOrder _order) {
  __c11_atomic_signal_fence(static_cast<int>(_order));
}

template<typename T>
inline void atomic_init(volatile AtomicBase<T>* base_, T _value) {
  __c11_atomic_init(&base_->value, _value);
}

template<typename T>
inline void atomic_init(AtomicBase<T>* base_, T _value) {
  __c11_atomic_init(&base_->value, _value);
}

template<typename T>
inline void atomic_store(volatile AtomicBase<T>* base_, T _value,
  MemoryOrder _order)
{
  __c11_atomic_store(&base_->value, _value, static_cast<int>(_order));
}

template<typename T>
inline void atomic_store(AtomicBase<T>* base_, T _value, MemoryOrder _order) {
  __c11_atomic_store(&base_->value, _value, static_cast<int>(_order));
}

template<typename T>
inline T atomic_load(const volatile AtomicBase<T>* base_,
MemoryOrder _order)
{
  using ptr_type = traits::remove_const<decltype(base_->value)>*;
  return __c11_atomic_load(const_cast<ptr_type>(&base_->value),
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_load(const AtomicBase<T>* base_, MemoryOrder _order) {
  using ptr_type = traits::remove_const<decltype(base_->value)>*;
  return __c11_atomic_load(const_cast<ptr_type>(&base_->value),
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_exchange(volatile AtomicBase<T>* base_, T _value,
  MemoryOrder _order)
{
  return __c11_atomic_exchange(&base_->value, _value, static_cast<int>(_order));
}

template<typename T>
inline T atomic_exchange(AtomicBase<T>* base_, T _value, MemoryOrder _order) {
  return __c11_atomic_exchange(&base_->value, _value, static_cast<int>(_order));
}

template<typename T>
inline bool atomic_compare_exchange_strong(volatile AtomicBase<T>* base_,
  T* _expected, T _value, MemoryOrder _order)
{
  return __c11_atomic_compare_exchange_strong(&base_->value, _expected, _value,
    static_cast<int>(_order));
}

template<typename T>
inline bool atomic_compare_exchange_strong(AtomicBase<T>* base_, T* _expected,
  T _value, MemoryOrder _order)
{
  return __c11_atomic_compare_exchange_strong(&base_->value, _expected, _value,
    static_cast<int>(_order));
}

template<typename T>
inline bool atomic_compare_exchange_weak(volatile AtomicBase<T>* base_,
  T* _expected, T _value, MemoryOrder _order)
{
  return __c11_atomic_compare_exchange_weak(&base_->value, _expected, _value,
    static_cast<int>(_order));
}

template<typename T>
inline bool atomic_compare_exchange_weak(AtomicBase<T>* base_, T* _expected,
  T _value, MemoryOrder _order)
{
  return __c11_atomic_compare_exchange_weak(&base_->value, _expected, _value,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_add(volatile AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_add(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_add(AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_add(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T* atomic_fetch_add(volatile AtomicBase<T>* base_, rx_ptrdiff _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_add(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T* atomic_fetch_add(AtomicBase<T>* base_, rx_ptrdiff _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_add(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_sub(volatile AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_sub(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_sub(AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_sub(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T* atomic_fetch_sub(volatile AtomicBase<T>* base_, rx_ptrdiff _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_sub(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T* atomic_fetch_sub(AtomicBase<T>* base_, rx_ptrdiff _delta,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_sub(&base_->value, _delta,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_and(volatile AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_and(&base_->value, _pattern,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_and(AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_and(&base_->value, _pattern,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_or(volatile AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_or(&base_->value, _pattern,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_or(AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_or(&base_->value, _pattern,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_xor(volatile AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_xor(&base_->value, _pattern,
    static_cast<int>(_order));
}

template<typename T>
inline T atomic_fetch_xor(AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return __c11_atomic_fetch_xor(&base_->value, _pattern,
    static_cast<int>(_order));
}

} // namespace rx::concurrency::detail

#endif // defined(RX_COMPILER_CLANG)
#endif // RX_CORE_CONCURRENCY_CLANG_ATOMIC_H

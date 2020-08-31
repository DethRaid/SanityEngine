#ifndef RX_CORE_CONCURRENCY_STD_ATOMIC_H
#define RX_CORE_CONCURRENCY_STD_ATOMIC_H
#include <atomic>

#include "rx/core/hints/unreachable.h"

namespace Rx::Concurrency::detail {

static inline constexpr std::memory_order convert_memory_order(MemoryOrder _order) {
  switch (_order) {
  case MemoryOrder::k_relaxed:
    return std::memory_order_relaxed;
  case MemoryOrder::k_consume: // load-consume
    return std::memory_order_consume;
  case MemoryOrder::k_acquire: // load-acquire
    return std::memory_order_acquire;
  case MemoryOrder::k_release: // store-release
    return std::memory_order_release;
  case MemoryOrder::k_acq_rel: // store-release load-acquire
    return std::memory_order_acq_rel;
  case MemoryOrder::k_seq_cst: // store-release load-acquire
    return std::memory_order_seq_cst;
  }
  RX_HINT_UNREACHABLE();
}

template<typename T>
struct AtomicBase {
  AtomicBase() = default;
  constexpr explicit AtomicBase(T _value) : value(_value) {}
  std::atomic<T> value;
};

template<typename T>
inline void atomic_init(volatile AtomicBase<T>* base_, T _value) {
  std::atomic_init(base_->value, _value);
}

template<typename T>
inline void atomic_init(AtomicBase<T>* base_, T _value) {
  std::atomic_init(base_->value, _value);
}

inline void atomic_thread_fence(MemoryOrder _order) {
  std::atomic_thread_fence(convert_memory_order(_order));
}

inline void atomic_signal_fence(MemoryOrder _order) {
  std::atomic_signal_fence(convert_memory_order(_order));
}

template<typename T>
inline void atomic_store(volatile AtomicBase<T>* base_, T _value,
  MemoryOrder _order)
{
  std::atomic_store_explicit(&base_->value, _value, convert_memory_order(_order));
}

template<typename T>
inline void atomic_store(AtomicBase<T>* base_, T _value, MemoryOrder _order) {
  std::atomic_store_explicit(&base_->value, _value, convert_memory_order(_order));
}

template<typename T>
inline T atomic_load(const volatile AtomicBase<T>* base_,
  MemoryOrder _order)
{
  return std::atomic_load_explicit(&base_->value, convert_memory_order(_order));
}

template<typename T>
inline T atomic_load(const AtomicBase<T>* base_, MemoryOrder _order) {
  return std::atomic_load_explicit(&base_->value, convert_memory_order(_order));
}

template<typename T>
inline T atomic_exchange(volatile AtomicBase<T>* base_, T _value,
  MemoryOrder _order)
{
  return std::atomic_exchange_explicit(&base_->value, _value, convert_memory_order(_order));
}

template<typename T>
inline T atomic_exchange(AtomicBase<T>* base_, T _value, MemoryOrder _order) {
  return std::atomic_exchange_explicit(&base_->value, _value, convert_memory_order(_order));
}

template<typename T>
inline bool atomic_compare_exchange_strong(volatile AtomicBase<T>* base_,
  T* _expected, T _value, MemoryOrder _order)
{
  return std::atomic_compare_exchange_strong_explicit(&base_->value, _expected, _value, convert_memory_order(_order));
}

template<typename T>
inline bool atomic_compare_exchange_strong(AtomicBase<T>* base_, T* _expected,
  T _value, MemoryOrder _order)
{
  return std::atomic_compare_exchange_strong_explicit(&base_->value, _expected, _value, convert_memory_order(_order));
}

template<typename T>
inline bool atomic_compare_exchange_weak(volatile AtomicBase<T>* base_,
  T* _expected, T _value, MemoryOrder _order)
{
  return std::atomic_compare_exchange_weak_explicit(&base_->value, _expected, _value, convert_memory_order(_order));
}

template<typename T>
inline bool atomic_compare_exchange_weak(AtomicBase<T>* base_, T* _expected,
  T _value, MemoryOrder _order)
{
  return std::atomic_compare_exchange_weak_explicit(&base_->value, _expected, _value, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_add(volatile AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_add_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_add(AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_add_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T* atomic_fetch_add(volatile AtomicBase<T>* base_, PtrDiff _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_add_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T* atomic_fetch_add(AtomicBase<T>* base_, PtrDiff _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_add_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_sub(volatile AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_sub_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_sub(AtomicBase<T>* base_, T _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_sub_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T* atomic_fetch_sub(volatile AtomicBase<T>* base_, PtrDiff _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_sub_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T* atomic_fetch_sub(AtomicBase<T>* base_, PtrDiff _delta,
  MemoryOrder _order)
{
  return std::atomic_fetch_sub_explicit(&base_->value, _delta, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_and(volatile AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return std::atomic_fetch_and_explicit(&base_->value, _pattern, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_and(AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return std::atomic_fetch_and_explicit(&base_->value, _pattern, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_or(volatile AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return std::atomic_fetch_or_explicit(&base_->value, _pattern, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_or(AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return std::atomic_fetch_or_explicit(&base_->value, _pattern, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_xor(volatile AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return std::atomic_fetch_xor_explicit(&base_->value, _pattern, convert_memory_order(_order));
}

template<typename T>
inline T atomic_fetch_xor(AtomicBase<T>* base_, T _pattern,
  MemoryOrder _order)
{
  return std::atomic_fetch_xor_explicit(&base_->value, _pattern, convert_memory_order(_order));
}

} // namespace Rx::Concurrency::detail

#endif // RX_CORE_CONCURRENCY_STD_ATOMIC_H

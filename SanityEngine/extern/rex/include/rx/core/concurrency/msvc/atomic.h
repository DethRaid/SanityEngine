#ifndef RX_CORE_CONCURRENCY_MSVC_ATOMIC_H
#define RX_CORE_CONCURRENCY_MSVC_ATOMIC_H
#if defined(RX_COMPILER_MSVC)

#include <atomic>

#include "rx/core/traits/remove_const.h"

namespace Rx::Concurrency::detail {
    template <typename T>
    using AtomicBase = std::atomic<T>;

    inline std::memory_order to_memory_order(const MemoryOrder order) {
        switch(order) {
            case MemoryOrder::k_relaxed:
                return std::memory_order::memory_order_relaxed;

            case MemoryOrder::k_consume:
                return std::memory_order::memory_order_consume;

            case MemoryOrder::k_acquire:
                return std::memory_order::memory_order_acquire;

            case MemoryOrder::k_release:
                return std::memory_order::memory_order_release;

            case MemoryOrder::k_acq_rel:
                return std::memory_order::memory_order_acq_rel;

            case MemoryOrder::k_seq_cst:
                return std::memory_order::memory_order_seq_cst;

            default:
                return std::memory_order::memory_order_relaxed;
        }
    }

    inline void atomic_thread_fence(MemoryOrder _order) {}

    inline void atomic_signal_fence(MemoryOrder _order) {}

    template <typename T>
    inline void atomic_init(volatile AtomicBase<T>* base_, T _value) {
        base_->store(_value);
    }

    template <typename T>
    inline void atomic_init(AtomicBase<T>* base_, T _value) {
        base_->store(_value);
    }

    template <typename T>
    inline void atomic_store(volatile AtomicBase<T>* base_, T _value, MemoryOrder _order) {
        base_->store(_value, to_memory_order(_order));
    }

    template <typename T>
    inline void atomic_store(AtomicBase<T>* base_, T _value, MemoryOrder _order) {
        base_->store(_value, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_load(const volatile AtomicBase<T>* base_, MemoryOrder _order) {
        return base_->load(to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_load(const AtomicBase<T>* base_, MemoryOrder _order) {
        return base_->load(to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_exchange(volatile AtomicBase<T>* base_, T _value, MemoryOrder _order) {
        return base_->exchange(_value, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_exchange(AtomicBase<T>* base_, T _value, MemoryOrder _order) {
        return base_->exchange(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong(volatile AtomicBase<T>* base_, T* _expected, T _value, MemoryOrder _order) {
        return base_->compare_exchange_strong(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_strong(AtomicBase<T>* base_, T* _expected, T _value, MemoryOrder _order) {
        return base_->compare_exchange_strong(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak(volatile AtomicBase<T>* base_, T* _expected, T _value, MemoryOrder _order) {
        return base_->compare_exchange_weak(_value, to_memory_order(_order));
    }

    template <typename T>
    inline bool atomic_compare_exchange_weak(AtomicBase<T>* base_, T* _expected, T _value, MemoryOrder _order) {
        return base_->compare_exchange_weak(_value, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_add(volatile AtomicBase<T>* base_, T _delta, MemoryOrder _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_add(AtomicBase<T>* base_, T _delta, MemoryOrder _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_add(volatile AtomicBase<T>* base_, PtrDiff _delta, MemoryOrder _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_add(AtomicBase<T>* base_, PtrDiff _delta, MemoryOrder _order) {
        return base_->fetch_add(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_sub(volatile AtomicBase<T>* base_, T _delta, MemoryOrder _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_sub(AtomicBase<T>* base_, T _delta, MemoryOrder _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_sub(volatile AtomicBase<T>* base_, PtrDiff _delta, MemoryOrder _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T* atomic_fetch_sub(AtomicBase<T>* base_, PtrDiff _delta, MemoryOrder _order) {
        return base_->fetch_sub(_delta, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_and(volatile AtomicBase<T>* base_, T _pattern, MemoryOrder _order) {
        return base_->fetch_and(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_and(AtomicBase<T>* base_, T _pattern, MemoryOrder _order) {
        return base_->fetch_and(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_or(volatile AtomicBase<T>* base_, T _pattern, MemoryOrder _order) {
        return base_->fetch_or(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_or(AtomicBase<T>* base_, T _pattern, MemoryOrder _order) {
        return base_->fetch_or(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_xor(volatile AtomicBase<T>* base_, T _pattern, MemoryOrder _order) {
        return base_->fetch_xor(_pattern, to_memory_order(_order));
    }

    template <typename T>
    inline T atomic_fetch_xor(AtomicBase<T>* base_, T _pattern, MemoryOrder _order) {
        return base_->fetch_xor(_pattern, to_memory_order(_order));
    }
} // namespace Rx::Concurrency::detail

#endif // defined(RX_COMPILER_MSVC)
#endif // RX_CORE_CONCURRENCY_MSVC_ATOMIC_H

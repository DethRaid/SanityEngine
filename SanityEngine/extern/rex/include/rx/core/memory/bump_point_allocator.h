#ifndef RX_CORE_MEMORY_BUMP_POINT_ALLOCATOR_H
#define RX_CORE_MEMORY_BUMP_POINT_ALLOCATOR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/spin_lock.h"

namespace Rx::Memory {

// # Bump Point Allocator
//
// The idea behind a bump point allocator is to start with a pointer at the
// beginning of a fixed-sized block of memory and bump it forward by the
// size of an allocation. This is a very basic, but fast allocation strategy.
//
// Unlike other allocation algorithms, a bump point allocator has an extremely
// fast way to deallocate everything, just reset the bump pointer to the
// beginning of the fixed-size block of memory.
//
// The implementation here is a bit more intelligent though since it supports
// reallocation and deallocation provided the pointer passed to either is the
// same as the last pointer returned by a call to allocate.
//
// The purpose of this allocator is to provide a very quick, linear burn
// scratch space to allocate shortly-lived objects and to reset.
struct RX_API BumpPointAllocator
  final : Allocator
{
  BumpPointAllocator(Byte* _memory, Size _size);

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* data);

  void reset();

  Size used() const;
  Size size() const;
  Size available() const;

private:
  Byte* allocate_unlocked(Size _size);

  Size m_size;
  Byte* m_data;

  Concurrency::SpinLock m_lock;

  Byte* m_this_point RX_HINT_GUARDED_BY(m_lock);
  Byte* m_last_point RX_HINT_GUARDED_BY(m_lock);
};

inline Size BumpPointAllocator::used() const {
  return m_this_point - m_data;
}

inline Size BumpPointAllocator::size() const {
  return m_size;
}

inline Size BumpPointAllocator::available() const {
  return size() - used();
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_BUMP_POINT_ALLOCATOR_H

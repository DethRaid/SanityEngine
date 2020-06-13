#ifndef RX_CORE_MEMORY_HEAP_ALLOCATOR_H
#define RX_CORE_MEMORY_HEAP_ALLOCATOR_H
#include "rx/core/memory/allocator.h"

#include "rx/core/global.h"

namespace Rx::Memory {

// # Heap Allocator
//
// The generalized heap allocator. This is how memory is allocated from the
// operating system and returned to it. On most systems this just wraps the C
// standard functions: malloc, realloc and free.
//
// The purpose of this allocator is to provide a generic, heap allocator
// that can be used for anything.
struct HeapAllocator
  final : Allocator
{
  constexpr HeapAllocator() = default;

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* _data);

  static constexpr Allocator& instance();

private:
  static Global<HeapAllocator> s_instance;
};

inline constexpr Allocator& HeapAllocator::instance() {
  return *s_instance;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_HEAP_ALLOCATOR_H

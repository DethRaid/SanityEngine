#ifndef RX_CORE_MEMORY_SINGLE_SHOT_ALLOCATOR_H
#define RX_CORE_MEMORY_SINGLE_SHOT_ALLOCATOR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/atomic.h"

namespace Rx::Memory {

// # Single Shot Allocator
//
// The idea behind a single shot allocator is to provide a single, one time use
// allocation from a fixed-size block of memory while still allowing that
// allocation to be resized and deallocated.
//
// When an allocation is made, no more allocations can be made until that
// allocation is deallocated.
//
// The single allocation can be resized in-place as many times as necessary
// provided the size is less than or equal to the fixed-size block.
//
// The purpose of this allocator is to enable containers which manage a single
// allocation, such as array and string to be made fixed-sized without introducing
// fixed-size variants.
struct SingleShotAllocator
  final : Allocator
{
  SingleShotAllocator(Byte* _data, Size _size);

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* _data);

private:
  Byte* m_data;
  Size m_size;
  Concurrency::Atomic<bool> m_allocated;
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SINGLE_SHOT_ALLOCATOR_H

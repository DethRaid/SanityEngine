#ifndef RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
#define RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H
#include "rx/core/concurrency/mutex.h"

#include "rx/core/memory/allocator.h"
#include "rx/core/memory/vma.h"

#include "rx/core/map.h"
#include "rx/core/global.h"

namespace Rx::Memory {

// # Electric Fence
//
// Special Type of allocator which uses VMAs to construct allocations that are
// surrounded by inacessible pages to help detect buffer under- and over- flows.
//
// This uses significant amounts of memory and should only be used for debugging
// memory corruption issues.
struct ElectricFenceAllocator
  final : Allocator
{
  ElectricFenceAllocator();

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* data);

  static constexpr Allocator& instance();

private:
  VMA* allocate_vma(Size _size);

  Concurrency::Mutex m_lock;
  Map<Byte*, VMA> m_mappings RX_HINT_GUARDED_BY(m_lock);

  static Global<ElectricFenceAllocator> s_instance;
};

inline constexpr Allocator& ElectricFenceAllocator::instance() {
  return *s_instance;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ELECTRIC_FENCE_ALLOCATOR_H

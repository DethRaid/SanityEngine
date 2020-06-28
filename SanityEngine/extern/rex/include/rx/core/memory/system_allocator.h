#ifndef RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#include "rx/core/memory/stats_allocator.h"

#include "rx/core/global.h"

namespace Rx::Memory {

// # System Allocator
//
// The generalized system allocator. Built off a heap allocator and a stats
// allocator to track global system allocations. When something isn't provided
// an allocator, this is the allocator used. More specifically, the global
// g_system_allocator is used.
struct SystemAllocator
  final : Allocator
{
  SystemAllocator();

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* _data);

  StatsAllocator::Statistics stats() const;

  static constexpr Allocator& instance();

private:
  StatsAllocator m_stats_allocator;

  static Global<SystemAllocator> s_instance;
};

inline StatsAllocator::Statistics SystemAllocator::stats() const {
  return m_stats_allocator.stats();
}

inline constexpr Allocator& SystemAllocator::instance() {
  return *s_instance;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_SYSTEM_ALLOCATOR_H

#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/electric_fence_allocator.h"
#include "rx/core/memory/heap_allocator.h"

namespace Rx::Memory {

SystemAllocator::SystemAllocator()
#if defined(RX_ESAN)
  : m_stats_allocator{ElectricFenceAllocator::instance()}
#else
  : m_stats_allocator{HeapAllocator::instance()}
#endif
{
}

Byte* SystemAllocator::allocate(Size _size) {
  return m_stats_allocator.allocate(_size);
}

Byte* SystemAllocator::reallocate(void* _data, Size _size) {
  return m_stats_allocator.reallocate(_data, _size);
}

void SystemAllocator::deallocate(void* _data) {
  return m_stats_allocator.deallocate(_data);
}

Global<SystemAllocator> SystemAllocator::s_instance{"system", "allocator"};

} // namespace rx::memory

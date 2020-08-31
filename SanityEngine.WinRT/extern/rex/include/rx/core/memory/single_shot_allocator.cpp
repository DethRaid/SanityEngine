#include "rx/core/memory/single_shot_allocator.h"

#include "rx/core/hints/likely.h"
#include "rx/core/hints/unlikely.h"

#include "rx/core/assert.h"

namespace Rx::Memory {

SingleShotAllocator::SingleShotAllocator(Byte* _data, Size _size)
  : m_data{_data}
  , m_size{_size}
  , m_allocated{false}
{
  // Ensure the memory given is suitably aligned and size is suitably rounded.
  RX_ASSERT(reinterpret_cast<UintPtr>(m_data) % k_alignment == 0,
    "_data not aligned on k_alignment boundary");
  RX_ASSERT(m_size % k_alignment == 0,
    "_size not a multiple of k_alignment");
}

Byte* SingleShotAllocator::allocate(Size _size) {
  // There's no need to round |_size| to alignment as only one allocation
  // is allowed.

  // Ensure there's enough space to satsify the allocation.
  if (RX_HINT_UNLIKELY(_size > m_size)) {
    return nullptr;
  }

  // Cannot allocate if already allocated.
  if (RX_HINT_UNLIKELY(m_allocated)) {
    return nullptr;
  }

  m_allocated = true;

  return m_data;
}

Byte* SingleShotAllocator::reallocate(void* _data, Size _size) {
  RX_ASSERT(m_allocated, "reallocate called before allocate");
  RX_ASSERT(_data == m_data, "invalid pointer");

  // Ensure there's enough space to satisfy the reallocation.
  if (RX_HINT_UNLIKELY(_size > m_size)) {
    return nullptr;
  }

  return m_data;
}

void SingleShotAllocator::deallocate(void* _data) {
  RX_ASSERT(m_allocated, "deallocate called before allocate");
  if (RX_HINT_LIKELY(_data)) {
    RX_ASSERT(_data == m_data, "invalid pointer");
    m_allocated = false;
  }
}

} // namespace rx::memory

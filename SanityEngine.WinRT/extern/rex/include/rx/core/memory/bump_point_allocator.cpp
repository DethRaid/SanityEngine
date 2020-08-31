#include <string.h> // memmove

#include "rx/core/memory/bump_point_allocator.h"
#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/hints/likely.h"
#include "rx/core/hints/unlikely.h"

#include "rx/core/assert.h"

namespace Rx::Memory {

BumpPointAllocator::BumpPointAllocator(Byte* _data, Size _size)
  : m_size{_size}
  , m_data{_data}
  , m_this_point{m_data}
  , m_last_point{m_data}
{
  RX_ASSERT(_data, "no memory supplied");

  // Ensure the memory given is suitably aligned and size is suitably rounded.
  RX_ASSERT(reinterpret_cast<UintPtr>(m_data) % k_alignment == 0,
    "_data not aligned on k_alignment boundary");
  RX_ASSERT(m_size % k_alignment == 0,
    "_size not a multiple of k_alignment");
}

Byte* BumpPointAllocator::allocate(Size _size) {
  Concurrency::ScopeLock locked{m_lock};
  return allocate_unlocked(_size);
}

Byte* BumpPointAllocator::allocate_unlocked(Size _size) {
  // Round |_size| to a multiple of k_alignment to keep all pointers
  // aligned by k_alignment.
  _size = Allocator::round_to_alignment(_size);

  // Check for available space for the allocation.
  if (RX_HINT_UNLIKELY(m_this_point + _size >= m_data + m_size)) {
    return nullptr;
  }

  // Backup the last point to make deallocation and reallocation possible.
  m_last_point = m_this_point;

  // Bump the point along by the rounded allocation size.
  m_this_point += _size;

  return m_last_point;
}

Byte* BumpPointAllocator::reallocate(void* _data, Size _size) {
  if (RX_HINT_LIKELY(_data)) {
    Concurrency::ScopeLock locked{m_lock};

    // Round |_size| to a multiple of k_alignment to keep all pointers
    // aligned by k_alignment.
    _size = Allocator::round_to_alignment(_size);

    // Can only reallocate in-place provided |_data| is the address of |m_last_point|,
    // i.e it's the most recently allocated.
    if (RX_HINT_LIKELY(_data == m_last_point)) {
      // Check for available space for the allocation.
      if (RX_HINT_UNLIKELY(m_last_point + _size >= m_data + m_size)) {
        // Out of memory.
        return nullptr;
      }

      // Bump the pointer along by the last point and new size.
      m_this_point = m_last_point + _size;

      return static_cast<Byte*>(_data);
    } else if (Byte* data = allocate_unlocked(_size)) {
      // This path is hit when resizing an allocation which isn't the last thing
      // allocated.
      //
      // We cannot tell how many bytes the original allocation was exactly as
      // there's no metadata describing it. Regardless, it's safe to copy past
      // the original allocation, potentially into another one, or ourselves,
      // as such copy represents uninitialized memory to the caller anyways.
      //
      // However, since it's possible for the copy to land into ourselves, we
      // cannot use memcpy here, use memmove instead.
      memmove(data, _data, _size);
      return data;
    } else {
      // Out of memory.
      return nullptr;
    }
  }

  return allocate(_size);
}

void BumpPointAllocator::deallocate(void* _data) {
  Concurrency::ScopeLock locked{m_lock};

  // Can only deallocate provided |_data| is the address of |m_last_point|,
  // i.e it's the most recently allocated or reallocated.
  if (RX_HINT_LIKELY(_data == m_last_point)) {
    m_this_point = m_last_point;
  }
}

void BumpPointAllocator::reset() {
  Concurrency::ScopeLock locked{m_lock};

  m_this_point = m_data;
  m_last_point = m_data;
}

} // namespace rx::memory

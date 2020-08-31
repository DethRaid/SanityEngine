#include "rx/core/memory/stats_allocator.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/hints/unlikely.h"
#include "rx/core/assert.h"

namespace Rx::Memory {

struct Header {
  // requested allocation size, the actual size is round_to_alignment(size) + sizeof(header) + k_alignment
  Size size;
  Byte* base;
};

Byte* StatsAllocator::allocate(Size _size) {
  const UintPtr size_as_multiple{round_to_alignment(_size)};
  const UintPtr actual_size{size_as_multiple + sizeof(Header) + k_alignment};

  Byte* base = m_allocator.allocate(actual_size);

  if (RX_HINT_UNLIKELY(!base)) {
    return nullptr;
  }

  Byte* aligned = reinterpret_cast<Byte*>(round_to_alignment(reinterpret_cast<UintPtr>(base) + sizeof(Header)));
  Header* node = reinterpret_cast<Header*>(aligned) - 1;
  node->size = _size;
  node->base = base;

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.allocations++;
    m_statistics.used_request_bytes += _size;
    m_statistics.used_actual_bytes += actual_size;
    m_statistics.peak_request_bytes = Algorithm::max(m_statistics.peak_request_bytes, m_statistics.used_request_bytes);
    m_statistics.peak_actual_bytes = Algorithm::max(m_statistics.peak_actual_bytes, m_statistics.used_actual_bytes);
  }
  return aligned;
}

Byte* StatsAllocator::reallocate(void* _data, Size _size) {
  if (RX_HINT_UNLIKELY(!_data)) {
    return allocate(_size);
  }

  const UintPtr size_as_multiple = round_to_alignment(_size);
  const UintPtr actual_size
    = size_as_multiple + sizeof(Header) + k_alignment;

  Header* node = reinterpret_cast<Header*>(_data) - 1;
  Byte* original = node->base;

  const Size original_request_size = node->size;
  const Size original_actual_size
    = round_to_alignment(node->size) + sizeof(Header) + k_alignment;

  Byte* resize = m_allocator.reallocate(original, actual_size);

  if (RX_HINT_UNLIKELY(!resize)) {
    return nullptr;
  }

  Byte* aligned =
    reinterpret_cast<Byte*>(round_to_alignment(reinterpret_cast<UintPtr>(resize) + sizeof(Header)));

  node = reinterpret_cast<Header*>(aligned) - 1;
  node->size = _size;
  node->base = resize;

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.request_reallocations++;
    if (resize == original) {
      m_statistics.actual_reallocations++;
    }
    m_statistics.used_request_bytes -= original_request_size;
    m_statistics.used_actual_bytes -= original_actual_size;
    m_statistics.used_request_bytes += _size;
    m_statistics.used_actual_bytes += actual_size;
  }
  return aligned;
}

void StatsAllocator::deallocate(void* _data) {
  if (RX_HINT_UNLIKELY(!_data)) {
    return;
  }

  Header* node = reinterpret_cast<Header*>(_data) - 1;
  const Size request_size = node->size;
  const Size actual_size
    = round_to_alignment(node->size) + sizeof(Header) + k_alignment;

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.deallocations++;
    m_statistics.used_request_bytes -= request_size;
    m_statistics.used_actual_bytes -= actual_size;
  }

  m_allocator.deallocate(node);
}

StatsAllocator::Statistics StatsAllocator::stats() const {
  // Hold a lock and make an entire copy of the structure atomically
  Concurrency::ScopeLock locked{m_lock};
  return m_statistics;
}

} // namespace rx::memory

#include "rx/core/memory/stats_allocator.h"
#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/algorithm/max.h"
#include "rx/core/hints/unlikely.h"
#include "rx/core/assert.h"

namespace Rx::Memory {

// We waste some memory here as the header must be a multiple of the |ALIGNMENT|
// yet |Header::request_bytes| is typically half the size of that.
//
// There's room to embed additional information here if we ever need to store
// more statistics about the given allocation.
struct alignas(Allocator::ALIGNMENT) Header {
  Size request_bytes;
};

static_assert(sizeof(Header) == Allocator::ALIGNMENT);
static_assert(alignof(Header) == Allocator::ALIGNMENT);

static constexpr inline Size actual_bytes_for_request(Size _request_bytes) {
  return Allocator::round_to_alignment(_request_bytes + sizeof(Header));
}

Byte* StatsAllocator::allocate(Size _request_bytes) {
  const auto actual_bytes = actual_bytes_for_request(_request_bytes);
  const auto base = m_allocator.allocate(actual_bytes);
  if (RX_HINT_UNLIKELY(!base)) {
    return nullptr;
  }

  const auto header = reinterpret_cast<Header*>(base);
  header->request_bytes = _request_bytes;

  const auto aligned = reinterpret_cast<Byte*>(header + 1);
  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.allocations++;
    m_statistics.used_request_bytes += _request_bytes;
    m_statistics.used_actual_bytes += actual_bytes;
    m_statistics.peak_request_bytes = Algorithm::max(m_statistics.peak_request_bytes, m_statistics.used_request_bytes);
    m_statistics.peak_actual_bytes = Algorithm::max(m_statistics.peak_actual_bytes, m_statistics.used_actual_bytes);
  }
  return aligned;
}

Byte* StatsAllocator::reallocate(void* _data, Size _new_request_bytes) {
  if (RX_HINT_UNLIKELY(!_data)) {
    return allocate(_new_request_bytes);
  }

  const auto old_header = reinterpret_cast<Header*>(_data) - 1;
  const auto old_request_bytes = old_header->request_bytes;
  const auto old_actual_bytes = actual_bytes_for_request(old_request_bytes);
  const auto old_base = reinterpret_cast<Byte*>(old_header);

  const auto new_actual_bytes = round_to_alignment(_new_request_bytes + sizeof(Header));
  const auto new_base = m_allocator.reallocate(old_base, new_actual_bytes);
  if (RX_HINT_UNLIKELY(!new_base)) {
    return nullptr;
  }

  const auto new_header = reinterpret_cast<Header*>(new_base);
  new_header->request_bytes = _new_request_bytes;

  const auto aligned = reinterpret_cast<Byte*>(new_header + 1);
  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.request_reallocations++;
    if (new_base == old_base) {
      m_statistics.actual_reallocations++;
    }
    m_statistics.used_request_bytes -= old_request_bytes;
    m_statistics.used_actual_bytes -= old_actual_bytes;
    m_statistics.used_request_bytes += _new_request_bytes;
    m_statistics.used_actual_bytes += new_actual_bytes;
  }

  return aligned;
}

void StatsAllocator::deallocate(void* _data) {
  if (RX_HINT_UNLIKELY(!_data)) {
    return;
  }

  const auto old_header = reinterpret_cast<Header*>(_data) - 1;
  const auto old_request_bytes = old_header->request_bytes;
  const auto old_actual_bytes = actual_bytes_for_request(old_request_bytes);
  const auto old_base = reinterpret_cast<Byte*>(old_header);

  {
    Concurrency::ScopeLock locked{m_lock};
    m_statistics.deallocations++;
    m_statistics.used_request_bytes -= old_request_bytes;
    m_statistics.used_actual_bytes -= old_actual_bytes;
  }

  m_allocator.deallocate(old_base);
}

StatsAllocator::Statistics StatsAllocator::stats() const {
  // Hold a lock and make an entire copy of the structure atomically
  Concurrency::ScopeLock locked{m_lock};
  return m_statistics;
}

} // namespace rx::memory

#ifndef RX_CORE_MEMORY_STATS_ALLOCATOR_H
#define RX_CORE_MEMORY_STATS_ALLOCATOR_H
#include "rx/core/memory/allocator.h"
#include "rx/core/concurrency/spin_lock.h"

namespace Rx::Memory {

// # Statistics Allocator
//
// The idea behind a statistics allocator is to wrap an existing allocator
// implementation and provide statistics for it. This is done by extending the
// size of the allocations to make room for additional meta-data.
//
// The purpose of this allocator is to provide a means to debug and track
// information about any allocator.
struct StatsAllocator
  final : Allocator
{
  constexpr StatsAllocator(Allocator& _allocator);

  virtual Byte* allocate(Size _size);
  virtual Byte* reallocate(void* _data, Size _size);
  virtual void deallocate(void* _data);

  struct Statistics {
    Size allocations;           // Number of calls to allocate
    Size request_reallocations; // Number of calls to reallocate in total
    Size actual_reallocations;  // Number of calls to reallocate that actually in-place reallocated
    Size deallocations;         // Number of calls to deallocate

    // Measures peak and in-use requested bytes.
    // Requested bytes are the sizes passed to allocate and reallocate.
    Uint64 peak_request_bytes;
    Uint64 used_request_bytes;

    // Measures peak and in-use actual bytes.
    // Actual bytes are the sizes once rounded and adjusted to make room for metadata.
    Uint64 peak_actual_bytes;
    Uint64 used_actual_bytes;
  };

  Statistics stats() const;

private:
  Allocator& m_allocator;
  mutable Concurrency::SpinLock m_lock;
  Statistics m_statistics; // protected by |m_lock|
};

inline constexpr StatsAllocator::StatsAllocator(Allocator& _allocator)
  : m_allocator{_allocator}
  , m_statistics{}
{
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_STATS_ALLOCATOR_H

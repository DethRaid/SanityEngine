#ifndef RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#define RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#include "rx/core/types.h"
#include "rx/core/markers.h"

#include "rx/core/utility/nat.h"

#include "rx/core/hints/force_inline.h"

namespace Rx::Memory {

// # Uninitialized storage
//
// Uninitialized storage of size |S| and alignment |A|.
//
// This is a convenience type that allows the storage to be used in constexpr
// contexts without actually initializing the contents of the storage.
template<Size S, Size A>
struct UninitializedStorage {
  RX_MARK_NO_COPY(UninitializedStorage);
  RX_MARK_NO_MOVE(UninitializedStorage);

  constexpr UninitializedStorage();

  constexpr Byte* data();
  constexpr const Byte* data() const;

private:
  union {
    Utility::Nat m_as_nat;
    alignas(A) Byte m_as_bytes[S];
  };
};

template<Size S, Size A>
RX_HINT_FORCE_INLINE constexpr UninitializedStorage<S, A>::UninitializedStorage()
  : m_as_nat{}
{
}

template<Size S, Size A>
RX_HINT_FORCE_INLINE constexpr Byte* UninitializedStorage<S, A>::data() {
  return m_as_bytes;
}

template<Size S, Size A>
RX_HINT_FORCE_INLINE constexpr const Byte* UninitializedStorage<S, A>::data() const {
  return m_as_bytes;
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H

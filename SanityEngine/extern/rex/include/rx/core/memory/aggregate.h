#ifndef RX_CORE_MEMORY_AGGREGATE_H
#define RX_CORE_MEMORY_AGGREGATE_H
#include "rx/core/assert.h"
#include "rx/core/utility/nat.h"

namespace Rx::Memory {

struct RX_API Aggregate {
  constexpr Aggregate();

  Size bytes() const;
  Size operator[](Size _index) const;

  template<typename T>
  bool add(Size _count);
  bool add(Size _size, Size _alignment, Size _count);
  bool finalize();

private:
  struct Entry {
    Size size;
    Size align;
    Size offset;
  };

  union {
    Utility::Nat m_nat;
    Entry m_entries[64];
  };

  Size m_size;
  Size m_bytes;
};

inline constexpr Aggregate::Aggregate()
  : m_nat{}
  , m_size{0}
  , m_bytes{0}
{
}

inline Size Aggregate::bytes() const {
  RX_ASSERT(m_bytes, "not finalized");
  return m_bytes;
}

inline Size Aggregate::operator[](Size _index) const {
  RX_ASSERT(m_bytes, "not finalized");
  return m_entries[_index].offset;
}

template<typename T>
inline bool Aggregate::add(Size _count) {
  return add(sizeof(T), alignof(T), _count);
}

} // namespace Rx::Memory

#endif // RX_CORE_MEMORY_AGGREGATE_H

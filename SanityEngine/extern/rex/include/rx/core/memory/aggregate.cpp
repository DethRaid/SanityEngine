#include "rx/core/memory/aggregate.h"
#include "rx/core/algorithm/max.h"

namespace Rx::Memory {

// The following calculation of padding for element offsets follows the same
// padding rules of structures in C/C++.
//
// The final alignment is based on which field has the largest alignment.
bool Aggregate::finalize() {
  if (m_bytes || m_size == 0) {
    return false;
  }

  auto align = [](Size _alignment, Size _offset) {
    return (_offset + (_alignment - 1)) & ~(_alignment - 1);
  };

  Size offset = 0;
  Size alignment = 0;
  for (Size i = 0; i < m_size; i++) {
    auto& entry = m_entries[i];
    const Size aligned = align(entry.align, offset);
    entry.offset = aligned;
    offset = aligned + entry.size;
    alignment = Algorithm::max(alignment, entry.align);
  }

  m_bytes = align(alignment, offset);

  return true;
}

bool Aggregate::add(Size _size, Size _alignment, Size _count) {
  RX_ASSERT(_size && _alignment, "empty field");
  RX_ASSERT(!m_bytes, "already finalized");

  // Would |_size * _count| overflow?
  if (_size && _count > -1_z / _size) {
    return false;
  }

  // Would another entry overflow |m_entries|?
  if (m_size >= sizeof m_entries / sizeof *m_entries) {
    return false;
  }

  m_entries[m_size++] = {_size * _count, _alignment, 0};
  return true;
}

} // namespace Rx::Memory

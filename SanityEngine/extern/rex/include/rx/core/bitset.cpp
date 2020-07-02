#include <string.h> // memset

#include "rx/core/bitset.h"
#include "rx/core/assert.h" // RX_ASSERT

#include "rx/core/memory/system_allocator.h"

namespace Rx {

Bitset::Bitset(Memory::Allocator& _allocator, Size _size)
  : m_allocator{&_allocator}
  , m_size{_size}
  , m_data{nullptr}
{
  m_data = reinterpret_cast<BitType*>(allocator().allocate(bytes_for_size(m_size)));
  RX_ASSERT(m_data, "out of memory");

  clear_all();
}

Bitset::Bitset(const Bitset& _bitset)
  : m_allocator{&_bitset.allocator()}
  , m_size{_bitset.m_size}
  , m_data{reinterpret_cast<BitType*>(allocator().allocate(bytes_for_size(m_size)))}
{
  RX_ASSERT(m_data, "out of memory");
  memcpy(m_data, _bitset.m_data, bytes_for_size(m_size));
}

Bitset& Bitset::operator=(Bitset&& bitset_) {
  RX_ASSERT(&bitset_ != this, "self assignment");

  m_allocator = &bitset_.allocator();
  m_size = Utility::exchange(bitset_.m_size, 0);
  m_data = Utility::exchange(bitset_.m_data, nullptr);

  return *this;
}

Bitset& Bitset::operator=(const Bitset& _bitset) {
  RX_ASSERT(&_bitset != this, "self assignment");

  allocator().deallocate(m_data);

  m_size = _bitset.m_size;
  m_data = reinterpret_cast<Uint64*>(allocator().allocate(bytes_for_size(m_size)));
  RX_ASSERT(m_data, "out of memory");

  memcpy(m_data, _bitset.m_data, bytes_for_size(m_size));

  return *this;
}

void Bitset::clear_all() {
  memset(m_data, 0, bytes_for_size(m_size));
}

Size Bitset::count_set_bits() const {
  Size count = 0;

  for (Size i = 0; i < m_size; i++) {
    if (test(i)) {
      count++;
    }
  }

  return count;
}

Size Bitset::count_unset_bits() const {
  Size count = 0;

  for (Size i = 0; i < m_size; i++) {
    if (!test(i)) {
      count++;
    }
  }

  return count;
}

Size Bitset::find_first_unset() const {
  for (Size i = 0; i < m_size; i++) {
    if (!test(i)) {
      return i;
    }
  }
  return -1_z;
}

Size Bitset::find_first_set() const {
  for (Size i = 0; i < m_size; i++) {
    if (test(i)) {
      return i;
    }
  }
  return -1_z;
}

} // namespace rx

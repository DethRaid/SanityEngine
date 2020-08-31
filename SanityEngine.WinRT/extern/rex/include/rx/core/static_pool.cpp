#include "rx/core/static_pool.h"

#include "rx/core/utility/exchange.h"

namespace Rx {

StaticPool::StaticPool(Memory::Allocator& _allocator, Size _object_size, Size _capacity)
  : m_allocator{&_allocator}
  , m_object_size{Memory::Allocator::round_to_alignment(_object_size)}
  , m_capacity{_capacity}
  , m_data{allocator().allocate(m_object_size, m_capacity)}
  , m_bitset{allocator(), m_capacity}
{
}

StaticPool::StaticPool(StaticPool&& pool_)
  : m_allocator{pool_.m_allocator}
  , m_object_size{Utility::exchange(pool_.m_object_size, 0)}
  , m_capacity{Utility::exchange(pool_.m_capacity, 0)}
  , m_data{Utility::exchange(pool_.m_data, nullptr)}
  , m_bitset{Utility::move(pool_.m_bitset)}
{
}

StaticPool& StaticPool::operator=(StaticPool&& pool_) {
  RX_ASSERT(&pool_ != this, "self assignment");

  m_allocator = pool_.m_allocator;
  m_object_size = Utility::exchange(pool_.m_object_size, 0);
  m_capacity = Utility::exchange(pool_.m_capacity, 0);
  m_data = Utility::exchange(pool_.m_data, nullptr);
  m_bitset = Utility::move(pool_.m_bitset);

  return *this;
}

Size StaticPool::allocate() {
  const Size index{m_bitset.find_first_unset()};
  if (RX_HINT_UNLIKELY(index == -1_z)) {
    return -1_z;
  }

  m_bitset.set(index);
  return index;
}

void StaticPool::deallocate(Size _index) {
  RX_ASSERT(m_bitset.test(_index), "unallocated");
  m_bitset.clear(_index);
}

} // namespace rx

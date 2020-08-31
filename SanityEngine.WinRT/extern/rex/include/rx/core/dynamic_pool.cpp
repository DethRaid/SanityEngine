#include "rx/core/dynamic_pool.h"

namespace Rx {

DynamicPool::DynamicPool(DynamicPool&& pool_)
  : m_allocator{pool_.m_allocator}
  , m_pools{Utility::move(pool_.m_pools)}
{
}

DynamicPool& DynamicPool::operator=(DynamicPool&& pool_) {
  m_allocator = pool_.m_allocator;
  m_pools = Utility::move(pool_.m_pools);
  return *this;
}

Size DynamicPool::pool_index_of(const Byte* _data) const {
  return m_pools.find_if([_data](const Ptr<StaticPool>& _pool) {
    return _pool->owns(_data);
  });
}

Byte* DynamicPool::data_of(Size _index) const {
  const Size pool_index = _index / m_pools.size();
  const Size object_index = _index % m_pools.size();
  return m_pools[pool_index]->data_of(object_index);
}

Size DynamicPool::index_of(const Byte* _data) const {
  if (const Size index = pool_index_of(_data); index != -1_z) {
    return index * m_pools.size();
  }
  return -1_z;
}

bool DynamicPool::add_pool() {
  auto pool = make_ptr<StaticPool>(allocator(), allocator(), m_object_size, m_objects_per_pool);
  return pool ? m_pools.push_back(Utility::move(pool)) : false;
}

} // namespace rx

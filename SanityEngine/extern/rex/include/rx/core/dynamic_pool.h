#ifndef RX_CORE_DYNAMIC_POOL_H
#define RX_CORE_DYNAMIC_POOL_H
#include "rx/core/static_pool.h"
#include "rx/core/vector.h"
#include "rx/core/ptr.h"

#include "rx/core/hints/empty_bases.h"

namespace Rx {

struct RX_HINT_EMPTY_BASES DynamicPool
  : Concepts::NoCopy
{
  constexpr DynamicPool(Memory::Allocator& _allocator, Size _object_size, Size _objects_per_pool);
  constexpr DynamicPool(Size _object_size, Size _per_pool);
  DynamicPool(DynamicPool&& pool_);

  DynamicPool& operator=(DynamicPool&& pool_);
  Byte* operator[](Size _index) const;

  Size allocate();
  void deallocate(Size _index);

  template<typename T, typename... Ts>
  T* create(Ts&&... _arguments);

  template<typename T>
  void destroy(T* _data);

  constexpr Memory::Allocator& allocator() const;

  Size object_size() const;
  Size size() const;

  Byte* data_of(Size _index) const;
  Size index_of(const Byte* _data) const;

private:
  [[nodiscard]] bool add_pool();
  Size pool_index_of(const Byte* _data) const;

  Ref<Memory::Allocator> m_allocator;
  Size m_object_size;
  Size m_objects_per_pool;
  Vector<Ptr<StaticPool>> m_pools;
};

inline constexpr DynamicPool::DynamicPool(Memory::Allocator& _allocator, Size _object_size, Size _objects_per_pool)
  : m_allocator{_allocator}
  , m_object_size{_object_size}
  , m_objects_per_pool{_objects_per_pool}
  , m_pools{allocator()}
{
}

inline constexpr DynamicPool::DynamicPool(Size _object_size, Size _per_pool)
  : DynamicPool{Memory::SystemAllocator::instance(), _object_size, _per_pool}
{
}

RX_HINT_FORCE_INLINE Byte* DynamicPool::operator[](Size _index) const {
  return data_of(_index);
}

template<typename T, typename... Ts>
inline T* DynamicPool::create(Ts&&... _arguments) {
  const Size pools = m_pools.size();
  for (Size i = 0; i < pools; i++) {
    auto& pool = m_pools[i];
    if (pool->can_allocate()) {
      return pool->create<T>(Utility::forward<Ts>(_arguments)...);
    }
  }

  if (add_pool()) {
    return create<T>(Utility::forward<Ts>(_arguments)...);
  }

  return nullptr;
}

template<typename T>
void DynamicPool::destroy(T* _data) {
  const Size index = pool_index_of(reinterpret_cast<const Byte*>(_data));
  if (index == -1_z) {
    return;
  }

  // Fetch the static pool with the given index, then destroy the data on
  // that pool, as it own's it.
  auto& pool = m_pools[index];
  pool->destroy<T>(_data);

  // When the pool is empty and it's the last pool in the list, to reduce
  // memory, remove it from |m_pools|.
  if (pool->is_empty() && pool == m_pools.last()) {
    m_pools.pop_back();
  }
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& DynamicPool::allocator() const {
  return m_allocator;
}

RX_HINT_FORCE_INLINE Size DynamicPool::object_size() const {
  return m_object_size;
}

RX_HINT_FORCE_INLINE Size DynamicPool::size() const {
  return m_pools.size();
}

} // namespace rx

#endif // RX_CORE_DYNAMIC_POOL_H

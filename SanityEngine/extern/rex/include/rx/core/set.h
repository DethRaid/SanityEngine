#ifndef RX_CORE_SET_H
#define RX_CORE_SET_H
#include "rx/core/hash.h"
#include "rx/core/array.h"

#include "rx/core/traits/is_trivially_destructible.h"
#include "rx/core/traits/return_type.h"
#include "rx/core/traits/is_same.h"

#include "rx/core/utility/swap.h"

#include "rx/core/hints/unreachable.h"

#include "rx/core/memory/system_allocator.h"
#include "rx/core/memory/aggregate.h"

namespace Rx {

// 32-bit: 28 bytes
// 64-bit: 56 bytes
template<typename K>
struct Set {
  template<typename Kt, Size E>
  using Initializers = Array<Kt[E]>;

  static inline constexpr Size k_initial_size{256};
  static inline constexpr Size k_load_factor{90};

  Set();
  Set(Memory::Allocator& _allocator);
  Set(Memory::Allocator& _allocator, const Set& _set);
  Set(Set&& set_);
  Set(const Set& _set);

  template<typename Kt, Size E>
  Set(Memory::Allocator& _allocator, Initializers<Kt, E>&& initializers_);

  template<typename Kt, Size E>
  Set(Initializers<Kt, E>&& initializers_);

  ~Set();

  Set& operator=(Set&& set_);
  Set& operator=(const Set& _set);

  K* insert(K&& _key);
  K* insert(const K& _key);

  K* find(const K& _key) const;

  bool erase(const K& _key);
  Size size() const;
  bool is_empty() const;

  void clear();

  template<typename F>
  bool each(F&& _function);
  template<typename F>
  bool each(F&& _function) const;

  constexpr Memory::Allocator& allocator() const;

private:
  void clear_and_deallocate();

  static Size hash_key(const K& _key);
  static bool is_deleted(Size _hash);

  Size desired_position(Size _hash) const;
  Size probe_distance(Size _hash, Size _slot_index) const;

  Size& element_hash(Size index);
  Size element_hash(Size index) const;

  [[nodiscard]] bool allocate();
  [[nodiscard]] bool grow();

  // move and non-move construction functions
  K* construct(Size _index, Size _hash, K&& key_);

  K* inserter(Size _hash, K&& key_);
  K* inserter(Size _hash, const K& _key);

  bool lookup_index(const K& _key, Size& _index) const;

  Memory::Allocator* m_allocator;

  union {
    Byte* m_data;
    K* m_keys;
  };
  Size* m_hashes;

  Size m_size;
  Size m_capacity;
  Size m_resize_threshold;
  Size m_mask;
};

template<typename K>
inline Set<K>::Set()
  : Set{Memory::SystemAllocator::instance()}
{
}

template<typename K>
inline Set<K>::Set(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_keys{nullptr}
  , m_hashes{nullptr}
  , m_size{0}
  , m_capacity{k_initial_size}
  , m_resize_threshold{0}
  , m_mask{0}
{
  RX_ASSERT(allocate(), "out of memory");
}

template<typename K>
inline Set<K>::Set(Set&& set_)
  : m_allocator{set_.m_allocator}
  , m_keys{Utility::exchange(set_.m_keys, nullptr)}
  , m_hashes{Utility::exchange(set_.m_hashes, nullptr)}
  , m_size{Utility::exchange(set_.m_size, 0)}
  , m_capacity{Utility::exchange(set_.m_capacity, k_initial_size)}
  , m_resize_threshold{Utility::exchange(set_.m_resize_threshold, 0)}
  , m_mask{Utility::exchange(set_.m_mask, 0)}
{
}

template<typename K>
inline Set<K>::Set(Memory::Allocator& _allocator, const Set& _set)
  : Set{_allocator}
{
  for (Size i{0}; i < _set.m_capacity; i++) {
    const auto hash = _set.element_hash(i);
    if (hash != 0 && !_set.is_deleted(hash)) {
      insert(_set.m_keys[i]);
    }
  }
}

template<typename K>
inline Set<K>::Set(const Set& _set)
  : Set{_set.allocator(), _set}
{
}

template<typename K>
template<typename Kt, Size E>
inline Set<K>::Set(Memory::Allocator& _allocator, Initializers<Kt, E>&& initializers_)
  : Set{_allocator}
{
  for (Size i = 0; i < E; i++) {
    insert(Utility::move(initializers_[i]));
  }
}

template<typename K>
template<typename Kt, Size E>
inline Set<K>::Set(Initializers<Kt, E>&& initializers_)
  : Set{Memory::SystemAllocator::instance(), Utility::move(initializers_)}
{
}

template<typename K>
inline Set<K>::~Set() {
  clear_and_deallocate();
}

template<typename K>
inline void Set<K>::clear() {
  if (m_size == 0) {
    return;
  }

  for (Size i{0}; i < m_capacity; i++) {
    const Size hash = element_hash(i);
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (!traits::is_trivially_destructible<K>) {
        Utility::destruct<K>(m_keys + i);
      }
      element_hash(i) = 0;
    }
  }

  m_size = 0;
}

template<typename K>
inline void Set<K>::clear_and_deallocate() {
  clear();

  allocator().deallocate(m_data);
}

template<typename K>
inline Set<K>& Set<K>::operator=(Set<K>&& set_) {
  RX_ASSERT(&set_ != this, "self assignment");

  clear_and_deallocate();

  m_allocator = set_.m_allocator;
  m_keys = Utility::exchange(set_.m_keys, nullptr);
  m_hashes = Utility::exchange(set_.m_hashes, nullptr);
  m_size = Utility::exchange(set_.m_size, 0);
  m_capacity = Utility::exchange(set_.m_capacity, 0);
  m_resize_threshold = Utility::exchange(set_.m_resize_threshold, 0);
  m_mask = Utility::exchange(set_.m_mask, 0);

  return *this;
}

template<typename K>
inline Set<K>& Set<K>::operator=(const Set<K>& _set) {
  RX_ASSERT(&_set != this, "self assignment");

  clear_and_deallocate();

  m_capacity = _set.m_capacity;
  RX_ASSERT(allocate(), "out of memory");

  for (Size i{0}; i < _set.m_capacity; i++) {
    const auto hash = _set.element_hash(i);
    if (hash != 0 && !_set.is_deleted(hash)) {
      insert(_set.m_keys[i]);
    }
  }

  return *this;
}

template<typename K>
inline K* Set<K>::insert(K&& key_) {
  if (++m_size >= m_resize_threshold && !grow()) {
    return nullptr;
  }
  return inserter(hash_key(key_), Utility::forward<K>(key_));
}

template<typename K>
inline K* Set<K>::insert(const K& _key) {
  if (++m_size >= m_resize_threshold && !grow()) {
    return nullptr;
  }
  return inserter(hash_key(_key), _key);
}

template<typename K>
K* Set<K>::find(const K& _key) const {
  if (Size index; lookup_index(_key, index)) {
    return m_keys + index;
  }
  return nullptr;
}

template<typename K>
inline bool Set<K>::erase(const K& _key) {
  if (Size index; lookup_index(_key, index)) {
    if constexpr (!traits::is_trivially_destructible<K>) {
      Utility::destruct<K>(m_keys + index);
    }

    if constexpr (sizeof index == 8) {
      element_hash(index) |= 0x8000000000000000;
    } else {
      element_hash(index) |= 0x80000000;
    }

    m_size--;
    return true;
  }
  return false;
}

template<typename K>
inline Size Set<K>::size() const {
  return m_size;
}

template<typename K>
inline bool Set<K>::is_empty() const {
  return m_size == 0;
}

template<typename K>
inline Size Set<K>::hash_key(const K& _key) {
  auto hash_value{Hash<K>{}(_key)};

  // MSB is used to indicate deleted elements
  if constexpr(sizeof hash_value == 8) {
    hash_value &= 0x7FFFFFFFFFFFFFFF_z;
  } else {
    hash_value &= 0x7FFFFFFF_z;
  }

  // don't ever hash to zero since zero is used to indicate unused slots
  hash_value |= hash_value == 0;

  return hash_value;
}

template<typename K>
inline bool Set<K>::is_deleted(Size _hash) {
  // MSB indicates tombstones
  return (_hash >> ((sizeof _hash * 8) - 1)) != 0;
}

template<typename K>
inline Size Set<K>::desired_position(Size _hash) const {
  return _hash & m_mask;
}

template<typename K>
inline Size Set<K>::probe_distance(Size _hash, Size _slot_index) const {
  return (_slot_index + m_capacity - desired_position(_hash)) & m_mask;
}

template<typename K>
inline Size& Set<K>::element_hash(Size _index) {
  return m_hashes[_index];
}

template<typename K>
inline Size Set<K>::element_hash(Size _index) const {
  return m_hashes[_index];
}

template<typename K>
inline bool Set<K>::allocate() {
  Memory::Aggregate aggregate;
  aggregate.add<K>(m_capacity);
  aggregate.add<Size>(m_capacity);
  aggregate.finalize();

  if (!(m_data = allocator().allocate(aggregate.bytes()))) {
    return false;
  }

  m_keys = reinterpret_cast<K*>(m_data + aggregate[0]);
  m_hashes = reinterpret_cast<Size*>(m_data + aggregate[1]);

  for (Size i{0}; i < m_capacity; i++) {
    element_hash(i) = 0;
  }

  m_resize_threshold = (m_capacity * k_load_factor) / 100;
  m_mask = m_capacity - 1;

  return true;
}

template<typename K>
inline bool Set<K>::grow() {
  const auto old_capacity{m_capacity};

  auto data = m_data;
  RX_ASSERT(data, "unallocated");

  auto keys_data = m_keys;
  auto hashes_data = m_hashes;

  m_capacity *= 2;
  if (!allocate()) {
    return false;
  }

  for (Size i{0}; i < old_capacity; i++) {
    const auto hash{hashes_data[i]};
    if (hash != 0 && !is_deleted(hash)) {
      RX_ASSERT(inserter(hash, Utility::move(keys_data[i])), "insertion failed");
      if constexpr (!traits::is_trivially_destructible<K>) {
        Utility::destruct<K>(keys_data + i);
      }
    }
  }

  allocator().deallocate(data);

  return true;
}

template<typename K>
inline K* Set<K>::construct(Size _index, Size _hash, K&& key_) {
  Utility::construct<K>(m_keys + _index, Utility::forward<K>(key_));
  element_hash(_index) = _hash;
  return m_keys + _index;
}

template<typename K>
inline K* Set<K>::inserter(Size _hash, K&& key_) {
  Size position = desired_position(_hash);
  Size distance = 0;

  K* result = nullptr;
  for (;;) {
    if (element_hash(position) == 0) {
      K* insert = construct(position, _hash, Utility::forward<K>(key_));
      return result ? result : insert;
    }

    const Size existing_element_probe_distance{probe_distance(element_hash(position), position)};
    if (existing_element_probe_distance < distance) {
      if (is_deleted(element_hash(position))) {
        K* insert = construct(position, _hash, Utility::forward<K>(key_));
        return result ? result : insert;
      }

      if (!result) {
        result = m_keys + position;
      }

      Utility::swap(_hash, element_hash(position));
      Utility::swap(key_, m_keys[position]);

      distance = existing_element_probe_distance;
    }

    position = (position + 1) & m_mask;
    distance++;
  }

  return result;
}

template<typename K>
inline K* Set<K>::inserter(Size _hash, const K& _key) {
  K key{_key};
  return inserter(_hash, Utility::move(key));
}

template<typename K>
inline bool Set<K>::lookup_index(const K& _key, Size& _index) const {
  const Size hash{hash_key(_key)};
  Size position{desired_position(hash)};
  Size distance{0};
  for (;;) {
    const Size hash_element{element_hash(position)};
    if (hash_element == 0) {
      return false;
    } else if (distance > probe_distance(hash_element, position)) {
      return false;
    } else if (hash_element == hash && m_keys[position] == _key) {
      _index = position;
      return true;
    }
    position = (position + 1) & m_mask;
    distance++;
  }

  RX_HINT_UNREACHABLE();
}

template<typename K>
template<typename F>
inline bool Set<K>::each(F&& _function) {
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (traits::is_same<traits::return_type<F>, bool>) {
        if (!_function(m_keys[i])) {
          return false;
        }
      } else {
        _function(m_keys[i]);
      }
    }
  }
  return true;
}

template<typename K>
template<typename F>
inline bool Set<K>::each(F&& _function) const {
  for (Size i{0}; i < m_capacity; i++) {
    const auto hash{m_hashes[i]};
    if (hash != 0 && !is_deleted(hash)) {
      if constexpr (traits::is_same<traits::return_type<F>, bool>) {
        if (!_function(m_keys[i])) {
          return false;
        }
      } else {
        _function(m_keys[i]);
      }
    }
  }
  return true;
}

template<typename K>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Set<K>::allocator() const {
  return *m_allocator;
}

} // namespace rx

#endif // RX_CORE_SET

#ifndef RX_CORE_ALGORITHM_TOPOLOGICAL_SORT_H
#define RX_CORE_ALGORITHM_TOPOLOGICAL_SORT_H
#include "rx/core/map.h"
#include "rx/core/set.h"
#include "rx/core/vector.h"

namespace Rx::Algorithm {

// # Topological Sort
//
// Really fast O(V+E), generic topological sorting using unordered hasing
// data structures.
//
// K must satisfy hashing and comparison with operator== for this to be used.
//
// Add nodes with |add(_key)|
// Add dependencies with |add(_key, _dependency)|
template<typename K>
struct TopologicalSort {
  TopologicalSort();
  TopologicalSort(Memory::Allocator& _allocator);

  struct Result {
    Vector<K> sorted; // sorted order of nodes
    Vector<K> cycled; // problem nodes that form cycles
  };

  bool add(const K& _key);
  bool add(const K& _key, const K& _dependency);

  Result sort();

  void clear();

  constexpr Memory::Allocator& allocator() const;

protected:
  struct Relations {
    Relations(Memory::Allocator& _allocator);
    Size dependencies;
    Set<K> dependents;
  };

  Ref<Memory::Allocator> m_allocator;
  Map<K, Relations> m_map;
};

template<typename K>
inline TopologicalSort<K>::TopologicalSort()
  : TopologicalSort{Memory::SystemAllocator::instance()}
{
}

template<typename K>
inline TopologicalSort<K>::TopologicalSort(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_map{allocator()}
{
}

template<typename K>
inline bool TopologicalSort<K>::add(const K& _key) {
  if (m_map.find(_key)) {
    return false;
  }
  return m_map.insert(_key, {allocator()}) != nullptr;
}

template<typename K>
inline bool TopologicalSort<K>::add(const K& _key, const K& _dependency) {
  // Cannot be a dependency of one-self.
  if (_key == _dependency) {
    return false;
  }

  // Dependents of the dependency.
  {
    auto find = m_map.find(_dependency);
    if (!find) {
      find = m_map.insert(_dependency, {allocator()});
    }
    auto& dependents = find->dependents;

    // Already a dependency.
    if (dependents.find(_key)) {
      return true;
    }

    dependents.insert(_key);
  }

  // Dependents of the key.
  {
    auto find = m_map.find(_key);
    if (!find) {
      find = m_map.insert(_key, {allocator()});
    }

    auto& dependencies = find->dependencies;

    // Another reference for this dependency.
    dependencies++;
  }

  return true;
}

template<typename K>
inline typename TopologicalSort<K>::Result TopologicalSort<K>::sort() {
  // Make a copy of the map because the sorting is destructive.
  auto map = m_map;

  Vector<K> sorted{allocator()};
  Vector<K> cycled{allocator()};

  // Each key that has no dependencies can be put in right away.
  map.each_pair([&](const K& _key, const Relations& _relations) {
    if (!_relations.dependencies) {
      sorted.push_back(_key);
    }
  });

  // Check dependents of the ones with no dependencies and store for each
  // resolved dependency.
  sorted.each_fwd([&](const K& _root_key) {
    map.find(_root_key)->dependents.each([&](const K& _key) {
      if (!--map.find(_key)->dependencies) {
        sorted.push_back(_key);
      }
    });
  });

  // When there's remaining dependencies of a relation then we've formed a cycle.
  map.each_pair([&](const K& _key, const Relations& _relations) {
    if (_relations.dependencies) {
      cycled.push_back(_key);
    }
  });

  return {Utility::move(sorted), Utility::move(cycled)};
}

template<typename T>
RX_HINT_FORCE_INLINE void TopologicalSort<T>::clear() {
  m_map.clear();
}

template<typename T>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& TopologicalSort<T>::allocator() const {
  return m_allocator;
}

template<typename T>
RX_HINT_FORCE_INLINE TopologicalSort<T>::Relations::Relations(Memory::Allocator& _allocator)
  : dependencies{0}
  , dependents{_allocator}
{
}

} // namespace rx::algorithm

#endif // RX_CORE_ALGORITHM_TOPOLIGCAL_SORT_H

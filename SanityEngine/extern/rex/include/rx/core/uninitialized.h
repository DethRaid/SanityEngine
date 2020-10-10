#ifndef RX_CORE_UNINITIALIZED_H
#define RX_CORE_UNINITIALIZED_H
#include "rx/core/memory/uninitialized_storage.h"

#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"

namespace Rx {

// # Uninitialized object
//
// Represents an uninitialized object with explicit control over initialization
// and finalization.
//
// This allows embedding an object of some type in a class without actually
// initializing the object in the constructor of that class until later.
template<typename T>
struct Uninitialized {
  constexpr Uninitialized() = default;

  template<typename... Ts>
  void init(Ts&&... _args);
  void fini();

  T* data();
  const T* data() const;

private:
  Memory::UninitializedStorage<sizeof(T), alignof(T)> m_storage;
};

template<typename T>
template<typename... Ts>
inline void Uninitialized<T>::init(Ts&&... _arguments) {
  Utility::construct<T>(m_storage.data(), Utility::forward<Ts>(_arguments)...);
}

template<typename T>
inline void Uninitialized<T>::fini() {
  Utility::destruct<T>(m_storage.data());
}

template<typename T>
RX_HINT_FORCE_INLINE T* Uninitialized<T>::data() {
  return reinterpret_cast<T*>(m_storage.data());
}

template<typename T>
RX_HINT_FORCE_INLINE const T* Uninitialized<T>::data() const {
  return reinterpret_cast<const T*>(m_storage.data());
}

} // namespace Rx::Memory

#endif // RX_CORE_UNINITIALIZED_H

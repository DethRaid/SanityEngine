#ifndef RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#define RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H
#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"
#include "rx/core/utility/forward.h"
#include "rx/core/utility/nat.h"

#include "rx/core/concepts/no_copy.h"
#include "rx/core/concepts/no_move.h"

#include "rx/core/hints/empty_bases.h"

namespace Rx::Memory {

// represents uninitialized storage suitable in size and alignment for
// an object of Type |T|, can be Type erased for implementing deferred static
// globals and variant types
template<typename T>
struct RX_HINT_EMPTY_BASES UninitializedStorage
  : Concepts::NoCopy
  , Concepts::NoMove
{
  constexpr UninitializedStorage();

  // explicitly initialize the storage with |args|
  template<typename... Ts>
  void init(Ts&&... _args);

  // explicitly finalize the storage
  void fini();

  // get the storage
  T* data();
  const T* data() const;

private:
  union {
    Utility::Nat m_nat;
    alignas(T) mutable Byte m_data[sizeof(T)];
  };
};

// uninitialized_storage
template<typename T>
inline constexpr UninitializedStorage<T>::UninitializedStorage()
  : m_nat{}
{
}

template<typename T>
template<typename... Ts>
inline void UninitializedStorage<T>::init(Ts&&... _args) {
  Utility::construct<T>(reinterpret_cast<void*>(m_data), Utility::forward<Ts>(_args)...);
}

template<typename T>
inline void UninitializedStorage<T>::fini() {
  Utility::destruct<T>(reinterpret_cast<void*>(m_data));
}

template<typename T>
inline T* UninitializedStorage<T>::data() {
  return reinterpret_cast<T*>(m_data);
}

template<typename T>
inline const T* UninitializedStorage<T>::data() const {
  return reinterpret_cast<const T*>(m_data);
}

} // namespace rx::memory

#endif // RX_CORE_MEMORY_UNINITIALIZED_STORAGE_H

#ifndef RX_CORE_MEMORY_ALLOCATOR_H
#define RX_CORE_MEMORY_ALLOCATOR_H
#include "rx/core/utility/construct.h"
#include "rx/core/utility/destruct.h"

#include "rx/core/markers.h"

namespace Rx::Memory {

struct RX_API Allocator {
  RX_MARK_INTERFACE(Allocator);

  static inline constexpr const Size ALIGNMENT = 16;

  // allocate memory of size |_size|
  virtual Byte* allocate(Size _size) = 0;

  // reallocate existing memory |_data| to size |_size|, should be an alias for
  // allocate(_size) when |_data| is nullptr
  virtual Byte* reallocate(void* _data, Size _size) = 0;

  // reallocate existing memory |_data|
  virtual void deallocate(void* data) = 0;

  // create an object of Type |T| with constructor arguments |Ts| on this allocator
  template<typename T, typename... Ts>
  T* create(Ts&&... _arguments);

  // destroy an object of Type |T| on this allocator
  template<typename T>
  void destroy(void* _data);

  Byte* allocate(Size _size, Size _count);

  static constexpr UintPtr round_to_alignment(UintPtr _ptr_or_size);

  template<typename T>
  static Byte* round_to_alignment(T* _ptr);
};

inline constexpr UintPtr Allocator::round_to_alignment(UintPtr _ptr_or_size) {
  return (_ptr_or_size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

template<typename T>
inline Byte* Allocator::round_to_alignment(T* _ptr) {
  return reinterpret_cast<Byte*>(round_to_alignment(reinterpret_cast<UintPtr>(_ptr)));
}

template<typename T, typename... Ts>
inline T* Allocator::create(Ts&&... _arguments) {
  if (Byte* data = allocate(sizeof(T)); data) {
    return Utility::construct<T>(data, Utility::forward<Ts>(_arguments)...);
  }
  return nullptr;
}

template<typename T>
inline void Allocator::destroy(void* _data) {
  if (_data) {
    Utility::destruct<T>(_data);
    deallocate(_data);
  }
}

struct View {
  Allocator* owner;
  Byte* data;
  Size size;
};

} // namespace rx::memory

#endif // RX_CORE_MEMORY_ALLOCATOR_H

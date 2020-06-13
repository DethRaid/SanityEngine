#ifndef RX_CORE_MEMORY_ALLOCATOR_H
#define RX_CORE_MEMORY_ALLOCATOR_H
#include "rx/core/utility/construct.h" // utility::construct
#include "rx/core/utility/destruct.h" // utility::destruct

#include "rx/core/concepts/interface.h" // concepts::Interface

namespace Rx::Memory {

struct Allocator
  : Concepts::Interface
{
  // all allocators must align their data and round their sizes to this alignment
  // value as well, failure to do so will lead to unaligned reads and writes to
  // several engine interfaces that depend on this behavior and possible crashes
  // in interfaces that rely on alignment for SIMD and being able to tag pointer
  // bits with additional information.
  //
  // rounding of pointers and sizes can be done with round_to_alignment
  static constexpr const Size k_alignment = 16;

  constexpr Allocator() = default;
  ~Allocator() = default;

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

  static constexpr UintPtr round_to_alignment(UintPtr _ptr_or_size);
};

inline constexpr UintPtr Allocator::round_to_alignment(UintPtr _ptr_or_size) {
  return (_ptr_or_size + (k_alignment - 1)) & ~(k_alignment - 1);
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

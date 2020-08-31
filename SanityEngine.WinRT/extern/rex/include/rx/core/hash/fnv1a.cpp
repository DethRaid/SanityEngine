#include "rx/core/hash/fnv1a.h"
#include "rx/core/traits/is_same.h"

namespace Rx::Hash {

template<typename T>
T fnv1a(const Byte* _data, Size _size) {
  if constexpr (traits::is_same<T, Uint32>) {
    static constexpr const Uint32 k_prime = 0x1000193_u32;
    Uint32 hash = 0x811c9dc5_u32;
    for (Size i = 0; i < _size; i++) {
      hash = hash ^ _data[i];
      hash *= k_prime;
    }
  } else if constexpr (traits::is_same<T, Uint64>) {
    static constexpr const Uint64 k_prime = 0x100000001b3_u64;
    Uint64 hash = 0xcbf29ce484222325_u64;
    for (Size i = 0; i < _size; i++) {
      hash = hash ^ _data[i];
      hash *= k_prime;
    }
    return hash;
  }
  return 0;
}

template Uint32 fnv1a<Uint32>(const Byte* _data, Size _size);
template Uint64 fnv1a<Uint64>(const Byte* _data, Size _size);

} // namespace rx::hash

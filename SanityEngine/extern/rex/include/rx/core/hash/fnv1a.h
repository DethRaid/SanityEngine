#ifndef RX_CORE_HASH_FNV1A_H
#define RX_CORE_HASH_FNV1A_H
#include "rx/core/types.h"

// # Fowler-Noll-Vo hash

namespace Rx::Hash {

template<typename T>
RX_API T fnv1a(const Byte* _data, Size _size);

} // namespace rx::hash

#endif // RX_CORE_HASH_FNV1A_H

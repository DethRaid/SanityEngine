#ifndef RX_CORE_UTILITY_CONSTRUCT_H
#define RX_CORE_UTILITY_CONSTRUCT_H
#include "rx/core/types.h" // rx_size
#include "rx/core/utility/forward.h" // utility::forward
#include "rx/core/hints/force_inline.h"

struct RxPlacementNewTag {};

RX_HINT_FORCE_INLINE void* operator new(Size, void* _data, RxPlacementNewTag) {
  return _data;
}

namespace Rx::Utility {

template<typename T, typename... Ts>
RX_HINT_FORCE_INLINE T* construct(void* _data, Ts&&... _args) {
  return new (_data, RxPlacementNewTag{}) T{Utility::forward<Ts>(_args)...};
}

} // namespace rx::utility

#endif // RX_CORE_UTILITY_CONSTRUCT_H

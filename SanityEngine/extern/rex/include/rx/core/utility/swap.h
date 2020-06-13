#ifndef RX_CORE_UTILITY_SWAP_H
#define RX_CORE_UTILITY_SWAP_H
#include "rx/core/utility/move.h"

namespace Rx::Utility {

template<typename T>
inline void swap(T& lhs_, T& rhs_) {
  T tmp = Utility::move(lhs_);
  lhs_ = Utility::move(rhs_);
  rhs_ = Utility::move(tmp);
}

} // namespace rx::utility

#endif // RX_CORE_UTILITY_SWAP_H

#ifndef RX_CORE_ALGORITHM_QUICK_SORT_H
#define RX_CORE_ALGORITHM_QUICK_SORT_H
#include "rx/core/algorithm/insertion_sort.h"

#include "rx/core/utility/forward.h"
#include "rx/core/utility/swap.h"

namespace Rx::Algorithm {

template<typename T, typename F>
void quick_sort(T* start_, T* end_, F&& _compare) {
  while (end_ - start_ > 10) {
    T* middle = start_ + (end_ - start_) / 2;
    T* item1 = start_ + 1;
    T* item2 = end_ - 2;
    T pivot;

    if (_compare(*start_, *middle)) {
      // start < middle
      if (_compare(*(end_ - 1), *start_)) {
        // end < start < middle
        pivot = Utility::move(*start_);
        *start_ = Utility::move(*(end_ - 1));
        *(end_ - 1) = Utility::move(*middle);
      } else if (_compare(*(end_ - 1), *middle)) {
        // start <= end < middle
        pivot = Utility::move(*(end_ - 1));
        *(end_ - 1) = Utility::move(*middle);
      } else {
        pivot = Utility::move(*middle);
      }
    } else if (_compare(*start_, *(end_ - 1))) {
      // middle <= start <= end
      pivot = Utility::move(*start_);
      *start_ = Utility::move(*middle);
    } else if (_compare(*middle, *(end_ - 1))) {
      // middle < end <= start
      pivot = Utility::move(*(end_ - 1));
      *(end_ - 1) = Utility::move(*start_);
      *start_ = Utility::move(*middle);
    } else {
      pivot = Utility::move(*middle);
      Utility::swap(*start_, *(end_ - 1));
    }

    do {
      while (_compare(*item1, pivot)) {
        if (++item1 >= item2) {
          goto partitioned;
        }
      }

      while (_compare(pivot, *--item2)) {
        if (item1 >= item2) {
          goto partitioned;
        }
      }

      Utility::swap(*item1, *item2);
    } while (++item1 < item2);

partitioned:
    *(end_ - 2) = Utility::move(*item1);
    *item1 = Utility::move(pivot);

    if (item1 - start_ < end_ - item1 + 1) {
      quick_sort(start_, item1, Utility::forward<F>(_compare));
      start_ = item1 + 1;
    } else {
      quick_sort(item1 + 1, end_, Utility::forward<F>(_compare));
      end_ = item1;
    }
  }

  insertion_sort(start_, end_, Utility::forward<F>(_compare));
}

} // namespace rx::algorithm

#endif // RX_CORE_ALGORITHM_QUICK_SORT_H

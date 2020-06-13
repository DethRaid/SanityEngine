#ifndef RX_CORE_CONCURRENCY_WAIT_GROUP_H
#define RX_CORE_CONCURRENCY_WAIT_GROUP_H
#include "rx/core/types.h"

#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

namespace Rx::Concurrency {

struct WaitGroup {
  WaitGroup(Size _count);
  WaitGroup();

  void signal();
  void wait();

private:
  Size m_signaled_count; // protected by |m_mutex|
  Size m_count;          // protected by |m_mutex|
  Mutex m_mutex;
  ConditionVariable m_condition_variable;
};

inline WaitGroup::WaitGroup(Size _count)
  : m_signaled_count{0}
  , m_count{_count}
{
}

inline WaitGroup::WaitGroup()
  : WaitGroup{0}
{
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_WAIT_GROUP_H

#include "rx/core/concurrency/wait_group.h"
#include "rx/core/concurrency/scope_lock.h"

namespace Rx::Concurrency {

void WaitGroup::signal() {
  ScopeLock lock{m_mutex};
  m_signaled_count++;
  m_condition_variable.signal();
}

void WaitGroup::wait() {
  ScopeLock lock{m_mutex};
  m_condition_variable.wait(lock, [&]{ return m_signaled_count == m_count; });
}

} // namespace rx::concurrency

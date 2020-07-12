#ifndef RX_CORE_CONCURRENCY_THREAD_POOL_H
#define RX_CORE_CONCURRENCY_THREAD_POOL_H
#include "rx/core/intrusive_list.h"
#include "rx/core/function.h"
#include "rx/core/dynamic_pool.h"

#include "rx/core/concurrency/thread.h"
#include "rx/core/concurrency/mutex.h"
#include "rx/core/concurrency/condition_variable.h"

namespace Rx::Concurrency {

struct ThreadPool {
  RX_MARK_NO_COPY(ThreadPool);
  RX_MARK_NO_MOVE(ThreadPool);

  ThreadPool(Memory::Allocator& _allocator, Size _threads, Size _static_pool_size);
  ThreadPool(Size _threads, Size _job_pool_size);
  ~ThreadPool();

  // insert |_task| into the thread pool to be executed, the integer passed
  // to |_task| is the thread id of the calling thread in the pool
  void add(Function<void(int)>&& task_);

  constexpr Memory::Allocator& allocator() const;

  static constexpr ThreadPool& instance();

private:
  Memory::Allocator& m_allocator;

  Mutex m_mutex;
  ConditionVariable m_task_cond;
  ConditionVariable m_ready_cond;

  IntrusiveList m_queue     RX_HINT_GUARDED_BY(m_mutex);
  Vector<Thread> m_threads  RX_HINT_GUARDED_BY(m_mutex);
  DynamicPool m_job_memory  RX_HINT_GUARDED_BY(m_mutex);
  bool m_stop               RX_HINT_GUARDED_BY(m_mutex);

  static Global<ThreadPool> s_instance;
};

inline ThreadPool::ThreadPool(Size _threads, Size _static_pool_size)
  : ThreadPool{Memory::SystemAllocator::instance(), _threads, _static_pool_size}
{
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& ThreadPool::allocator() const {
  return m_allocator;
}

RX_HINT_FORCE_INLINE constexpr ThreadPool& ThreadPool::instance() {
  return *s_instance;
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_POOL_H

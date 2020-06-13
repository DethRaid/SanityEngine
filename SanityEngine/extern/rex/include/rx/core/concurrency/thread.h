#ifndef RX_CORE_CONCURRENCY_THREAD_H
#define RX_CORE_CONCURRENCY_THREAD_H
#include "rx/core/function.h"
#include "rx/core/ptr.h"

#include "rx/core/hints/empty_bases.h"

namespace Rx::Concurrency {

// NOTE: Thread names must have static-storage which lives as long as the thread.
// NOTR: Cannot deliver signals to threads.
struct RX_HINT_EMPTY_BASES Thread
  : Concepts::NoCopy
{
  template<typename F>
  Thread(Memory::Allocator& _allocator, const char* _name, F&& _function);

  template<typename F>
  Thread(const char* _name, F&& _function);

  Thread(Thread&& thread_);
  ~Thread();

  Thread& operator=(Thread&& thread_) = delete;

  void join();

  constexpr Memory::Allocator& allocator() const;

private:
  struct State {
    template<typename F>
    State(Memory::Allocator& _allocator, const char* _name, F&& _function);

    void spawn();
    void join();

  private:
    static void* wrap(void* _data);

    union {
      Utility::Nat m_nat;
      // Fixed-capacity storage for any OS thread Type, adjust if necessary.
      alignas(16) Byte m_thread[16];
    };

    Function<void(int)> m_function;
    const char* m_name;
    bool m_joined;
  };

  Ptr<State> m_state;
};

template<typename F>
inline Thread::State::State(Memory::Allocator& _allocator, const char* _name, F&& _function)
  : m_nat{}
  , m_function{_allocator, Utility::forward<F>(_function)}
  , m_name{_name}
  , m_joined{false}
{
  spawn();
}

template<typename F>
inline Thread::Thread(Memory::Allocator& _allocator, const char* _name, F&& _function)
  : m_state{make_ptr<State>(_allocator, _allocator, _name, Utility::forward<F>(_function))}
{
}

template<typename F>
inline Thread::Thread(const char* _name, F&& _function)
  : Thread{Memory::SystemAllocator::instance(), _name, Utility::forward<F>(_function)}
{
}

inline Thread::Thread(Thread&& thread_)
  : m_state{Utility::move(thread_.m_state)}
{
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Thread::allocator() const {
  return m_state.allocator();
}

} // namespace rx::concurrency

#endif // RX_CORE_CONCURRENCY_THREAD_H

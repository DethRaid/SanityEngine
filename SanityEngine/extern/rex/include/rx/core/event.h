#ifndef RX_CORE_EVENT_H
#define RX_CORE_EVENT_H
#include "rx/core/function.h"
#include "rx/core/vector.h"
#include "rx/core/markers.h"

#include "rx/core/concurrency/spin_lock.h"
#include "rx/core/concurrency/scope_lock.h"

#include "rx/core/utility/exchange.h"

namespace Rx {

template<typename T>
struct Event;

template<typename R, typename... Ts>
struct Event<R(Ts...)> {
  using Delegate = Function<R(Ts...)>;

  struct Handle {
    RX_MARK_NO_COPY(Handle);

    constexpr Handle(Event* _event, Size _index);
    constexpr Handle(Handle&& _existing)
        : m_event{Utility::exchange(_existing.m_event, nullptr)}, m_index{Utility::exchange(_existing.m_index, 0)} {};

    ~Handle();
  private:
    Event* m_event;
    Size m_index;
  };

  constexpr Event(Memory::Allocator& _allocator);
  constexpr Event();

  void signal(Ts... _arguments);
  Handle connect(Delegate&& function_);

  Size size() const;
  bool is_empty() const;

  constexpr Memory::Allocator& allocator() const;

private:
  friend struct Handle;
  mutable Concurrency::SpinLock m_lock;
  Vector<Delegate> m_delegates; // protected by |m_lock|
};

template<typename R, typename... Ts>
inline constexpr Event<R(Ts...)>::Handle::Handle(Event<R(Ts...)>* _event, Size _index)
  : m_event{_event}
  , m_index{_index}
{
}

template<typename R, typename... Ts>
inline Event<R(Ts...)>::Handle::~Handle() {
  if (m_event) {
    Concurrency::ScopeLock lock{m_event->m_lock};
    m_event->m_delegates[m_index] = nullptr;
  }
}

template<typename R, typename... Ts>
inline constexpr Event<R(Ts...)>::Event(Memory::Allocator& _allocator)
  : m_delegates{_allocator}
{
}

template<typename R, typename... Ts>
inline constexpr Event<R(Ts...)>::Event()
  : Event{Memory::SystemAllocator::instance()}
{
}

template<typename R, typename... Ts>
inline void Event<R(Ts...)>::signal(Ts... _arguments) {
  Concurrency::ScopeLock lock{m_lock};
  m_delegates.each_fwd([&](Delegate& _delegate) {
    if (_delegate) {
      _delegate(_arguments...);
    }
  });
}

template<typename R, typename... Ts>
inline typename Event<R(Ts...)>::Handle Event<R(Ts...)>::connect(Delegate&& delegate_) {
  Concurrency::ScopeLock lock{m_lock};
  const Size delegates = m_delegates.size();
  for (Size i{0}; i < delegates; i++) {
    if (!m_delegates[i]) {
      return {this, i};
    }
  }
  m_delegates.emplace_back(Utility::move(delegate_));
  return {this, delegates};
}

template<typename R, typename... Ts>
inline bool Event<R(Ts...)>::is_empty() const {
  return size() == 0;
}

template<typename R, typename... Ts>
inline Size Event<R(Ts...)>::size() const {
  Concurrency::ScopeLock lock{m_lock};
  // This is slightly annoying because |m_delegates| may have empty slots.
  Size result = 0;
  m_delegates.each_fwd([&](const Delegate& _delegate) {
    if (_delegate) {
      result++;
    }
  });
  return result;
}

template<typename R, typename... Ts>
RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Event<R(Ts...)>::allocator() const {
  return m_delegates.allocator();
}

} // namespace rx

#endif // RX_CORE_EVENT_H

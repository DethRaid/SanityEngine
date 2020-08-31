#ifndef RX_CORE_LOG_H
#define RX_CORE_LOG_H
#include "rx/core/event.h"
#include "rx/core/string.h"
#include "rx/core/source_location.h"

namespace Rx {

struct Stream;

struct Log {
  enum class Level {
    k_warning,
    k_info,
    k_verbose,
    k_error
  };

  using QueueEvent = Event<void(Level, String)>;
  using WriteEvent = Event<void(Level, String)>;
  using FlushEvent = Event<void()>;

  constexpr Log(const char* _name, const SourceLocation& _source_location);

  [[nodiscard]] static bool subscribe(Stream* _stream);
  [[nodiscard]] static bool unsubscribe(Stream* _stream);
  [[nodiscard]] static bool enqueue(Log* _owner, Level _level, String&& _contents);

  static void flush();

  // Write a formatted message given by |_format| and |_arguments| of associated
  // severity level |_level|. This will queue the given message on the logger
  // thread.
  //
  // All delegates given by |on_queue| will be called immediately by this
  // function (and thus on the same thread).
  //
  // This function is thread-safe.
  template<typename... Ts>
  bool write(Level _level, const char* _format, Ts&&... _arguments);

  // Convenience functions that call |write| with the appropriate severity
  // level, given by their name.
  //
  // All of these functions are thread-safe.
  template<typename... Ts>
  bool warning(const char* _format, Ts&&... _arguments);
  template<typename... Ts>
  bool info(const char* _format, Ts&&... _arguments);
  template<typename... Ts>
  bool verbose(const char* _format, Ts&&... _arguments);
  template<typename... Ts>
  bool error(const char* _format, Ts&&... _arguments);

  // When a message is queued, all delegates associated by this function are
  // called. This is different from |on_write| in that |callback_| is called
  // by the same thread which calls |write|, |warning|, |info|, |verbose|, or
  // |error| immediately.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called for such event.
  //
  // This function is thread-safe.
  QueueEvent::Handle on_queue(QueueEvent::Delegate&& callback_);

  // When a message is written, all delegates associated by this function are
  // called. This is different from |on_queue| in that |callback_| is called
  // by the logging thread when the actual message is written.
  //
  // This function returns the event handle, keep the handle alive for as
  // long as you want the delegate |callback_| to be called for such event.
  //
  // This function is thread-safe.
  WriteEvent::Handle on_write(WriteEvent::Delegate&& callback_);

  // When all messages queued for this log are actually written, all delegates
  // associated by this function are called.
  //
  // This function returns an event handle, keep the handle alive for as long
  // as you want the delegate |callback_| to be called for such event.
  //
  // This function is thread-safe.
  FlushEvent::Handle on_flush(FlushEvent::Delegate&& callback_);

  // Query the name of the logger, which is the name given to the |RX_LOG| macro.
  const char* name() const;

  // Query the source information of where this log is defined.
  const SourceLocation& source_info() const &;

  void signal_write(Level _level, String&& contents_);
  void signal_flush();

private:
  const char* m_name;
  SourceLocation m_source_location;

  QueueEvent m_queue_event;
  WriteEvent m_write_event;
  FlushEvent m_flush_event;
};

inline constexpr Log::Log(const char* _name, const SourceLocation& _source_location)
  : m_name{_name}
  , m_source_location{_source_location}
{
}

template<typename... Ts>
inline bool Log::write(Level _level, const char* _format, Ts&&... _arguments) {
  if constexpr (sizeof...(Ts) > 0) {
    auto format = String::format(_format, Utility::forward<Ts>(_arguments)...);
    m_queue_event.signal(_level, {format.allocator(), format});
    return enqueue(this, _level, Utility::move(format));
  } else {
    m_queue_event.signal(_level, _format);
    return enqueue(this, _level, {_format});
  }
}

template<typename... Ts>
inline bool Log::warning(const char* _format, Ts&&... _arguments) {
  return write(Level::k_warning, _format, Utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
inline bool Log::info(const char* _format, Ts&&... _arguments) {
  return write(Level::k_info, _format, Utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
inline bool Log::verbose(const char* _format, Ts&&... _arguments) {
  return write(Level::k_verbose, _format, Utility::forward<Ts>(_arguments)...);
}

template<typename... Ts>
inline bool Log::error(const char* _format, Ts&&... _arguments) {
  return write(Level::k_error, _format, Utility::forward<Ts>(_arguments)...);
}

inline const char* Log::name() const {
  return m_name;
}

inline const SourceLocation& Log::source_info() const & {
  return m_source_location;
}

inline Log::QueueEvent::Handle Log::on_queue(QueueEvent::Delegate&& callback_) {
  return m_queue_event.connect(Utility::move(callback_));
}

inline Log::WriteEvent::Handle Log::on_write(WriteEvent::Delegate&& callback_) {
  return m_write_event.connect(Utility::move(callback_));
}

inline Log::FlushEvent::Handle Log::on_flush(FlushEvent::Delegate&& callback_) {
  return m_flush_event.connect(Utility::move(callback_));
}

#define RX_LOG(_name, _identifier) \
  static ::Rx::Global<::Rx::Log> _identifier{"loggers", (_name), (_name), \
    ::Rx::SourceLocation{__FILE__, "(global constructor)", __LINE__}}

} // namespace rx

#endif // RX_CORE_LOG_H

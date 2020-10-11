#ifndef RX_CORE_STREAM_H
#define RX_CORE_STREAM_H
#include "rx/core/vector.h"
#include "rx/core/optional.h"

namespace Rx {

struct String;

struct RX_API Stream {
  RX_MARK_NO_COPY(Stream);

  // Stream flags.
  enum : Uint32 {
    k_read  = 1 << 0,
    k_write = 1 << 1,
    k_tell  = 1 << 2,
    k_seek  = 1 << 3,
    k_flush = 1 << 4
  };

  constexpr Stream(Uint32 _flags);
  Stream(Stream&& stream_);
  virtual ~Stream();

  enum class Whence {
    k_set,     // Beginning of stream.
    k_current, // Current position
    k_end      // End of stream.
  };

  [[nodiscard]] Uint64 read(Byte* _data, Uint64 _size);
  [[nodiscard]] Uint64 write(const Byte* _data, Uint64 _size);
  [[nodiscard]] bool seek(Sint64 _where, Whence _whence);
  [[nodiscard]] bool flush();

  Uint64 tell();
  Uint64 size();

  // Query the support of features on the given stream.
  constexpr bool can_read() const;
  constexpr bool can_write() const;
  constexpr bool can_tell() const;
  constexpr bool can_seek() const;
  constexpr bool can_flush() const;

  // The following functions below are what streams can implement. When a
  // stream cannot support a given feature, it should avoid providing a
  // function for it and ignore the flag when constructing this base class.

  // The name of the stream. This must always be implemented.
  virtual const String& name() const & = 0;

  // Read |_size| bytes from stream into |_data|.
  virtual Uint64 on_read(Byte* _data, Uint64 _size);

  // Write |_size| bytes from |_data| into stream.
  virtual Uint64 on_write(const Byte* _data, Uint64 _size);

  // Seek to |_where| in stream relative to |_whence|.
  virtual bool on_seek(Sint64 _where, Whence _whence);

  // Flush any buffered contents in the stream out.
  virtual bool on_flush();

  // Where we are in the stream.
  virtual Uint64 on_tell();

private:
  Uint32 m_flags;
};

inline constexpr Stream::Stream(Uint32 _flags)
  : m_flags{_flags}
{
}

inline Stream::Stream(Stream&& stream_)
  : m_flags{stream_.m_flags}
{
  stream_.m_flags = 0;
}

inline Stream::~Stream() = default;

bool inline constexpr Stream::can_read() const {
  return m_flags & k_read;
}

bool inline constexpr Stream::can_write() const {
  return m_flags & k_write;
}

bool inline constexpr Stream::can_tell() const {
  return m_flags & k_tell;
}

bool inline constexpr Stream::can_seek() const {
  return m_flags & k_seek;
}

bool inline constexpr Stream::can_flush() const {
  return m_flags & k_flush;
}

RX_API Optional<Vector<Byte>> read_binary_stream(Memory::Allocator& _allocator, Stream* _stream);
RX_API Optional<Vector<Byte>> read_text_stream(Memory::Allocator& _allocator, Stream* _stream);

} // namespace rx

#endif // RX_CORE_STREAM_H

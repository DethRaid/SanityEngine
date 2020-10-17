#ifndef RX_CORE_STREAM_H
#define RX_CORE_STREAM_H
#include "rx/core/vector.h"
#include "rx/core/optional.h"

namespace Rx {

struct String;

struct RX_API Stream {
  RX_MARK_NO_COPY(Stream);
  RX_MARK_NO_MOVE_ASSIGN(Stream);

  struct Stat {
    Uint64 size;
    // Can extend with aditional stats.
  };

  // Stream flags.
  enum : Uint32 {
    READ  = 1 << 0,
    WRITE = 1 << 1,
    SEEK  = 1 << 2,
    STAT  = 1 << 3
  };

  constexpr Stream(Uint32 _flags);
  Stream(Stream&& stream_);

  virtual ~Stream();

  enum class Whence {
    SET,     // Beginning of stream.
    CURRENT, // Current position
    END      // End of stream.
  };

  // Read |_size| bytes into |_data|. Returns the amount of bytes written.
  [[nodiscard]] Uint64 read(Byte* _data, Uint64 _size);

  // Write |_size| bytes from |_data|. Returns the amount of bytes written.
  [[nodiscard]] Uint64 write(const Byte* _data, Uint64 _size);

  // Seek stream |_where| bytes relative to |_whence|. Returns true on success.
  [[nodiscard]] bool seek(Sint64 _where, Whence _whence);

  // Stat stream for info.
  [[nodiscard]] Optional<Stat> stat() const;

  // Where in the stream we are.
  Uint64 tell() const;

  // The size of the stream.
  Optional<Uint64> size() const;

  // Supported stream functions.
  constexpr bool can_read() const;
  constexpr bool can_write() const;
  constexpr bool can_seek() const;
  constexpr bool can_stat() const;

  // The name of the stream. This must always be implemented.
  virtual const String& name() const & = 0;

protected:
  // Read |_size| bytes from stream into |_data|.
  virtual Uint64 on_read(Byte* _data, Uint64 _size);

  // Write |_size| bytes from |_data| into stream.
  virtual Uint64 on_write(const Byte* _data, Uint64 _size);

  // Seek to |_where| in stream.
  virtual bool on_seek(Uint64 _where);

  // Stat the stream.
  virtual bool on_stat(Stat& stat_) const;

private:
  Uint32 m_flags;
  Uint64 m_offset;
};

inline constexpr Stream::Stream(Uint32 _flags)
  : m_flags{_flags}
  , m_offset{0}
{
}

inline Stream::Stream(Stream&& stream_)
  : m_flags{Utility::exchange(stream_.m_flags, 0)}
  , m_offset{Utility::exchange(stream_.m_offset, 0)}
{
}

inline Stream::~Stream() = default;

inline Uint64 Stream::tell() const {
  return m_offset;
}

inline Optional<Uint64> Stream::size() const {
  if (auto s = stat()) {
    return s->size;
  }
  return nullopt;
}

inline constexpr bool Stream::can_read() const {
  return m_flags & READ;
}

inline constexpr bool Stream::can_write() const {
  return m_flags & WRITE;
}

inline constexpr bool Stream::can_seek() const {
  return m_flags & SEEK;
}

inline constexpr bool Stream::can_stat() const {
  return m_flags & STAT;
}

RX_API Optional<Vector<Byte>> read_binary_stream(Memory::Allocator& _allocator, Stream* _stream);
RX_API Optional<Vector<Byte>> read_text_stream(Memory::Allocator& _allocator, Stream* _stream);

} // namespace rx

#endif // RX_CORE_STREAM_H

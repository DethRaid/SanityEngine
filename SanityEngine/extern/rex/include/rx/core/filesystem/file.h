#ifndef RX_CORE_FILESYSTEM_FILE_H
#define RX_CORE_FILESYSTEM_FILE_H
#include "rx/core/stream.h"
#include "rx/core/string.h"

#include "rx/core/utility/exchange.h"

namespace Rx::Filesystem {

struct RX_API File
  final : Stream
{
  constexpr File();
  constexpr File(Memory::Allocator& _allocator);
  File(Memory::Allocator& _allocator, const char* _file_name, const char* _mode);
  File(Memory::Allocator& _allocator, const String& _file_name, const char* _mode);
  File(const char* _file_name, const char* _mode);
  File(const String& _file_name, const char* _mode);
  File(File&& other_);
  ~File();

  File& operator=(File&& file_);

  bool read_line(String& line_);
  bool close();

  // Print |_fmt| with |_args| to file using |_allocator| for formatting.
  // NOTE: asserts if the file isn't a text file.
  template<typename... Ts>
  bool print(Memory::Allocator& _allocator, const char* _fmt, Ts&&... _args);

  // Print |_fmt| with |_args| to file using system allocator for formatting.
  // NOTE: asserts if the file isn't a text file.
  template<typename... Ts>
  bool print(const char* _fmt, Ts&&... _args);

  // Print a string into the file. This is only valid for text files.
  // NOTE: asserts if the file isn't a text file.
  bool print(String&& contents_);

  // Query if the file handle is valid, will be false if the file has been
  // closed with |close| or if the file failed to open.
  bool is_valid() const;

  operator bool() const;

  virtual const String& name() const &;

  constexpr Memory::Allocator& allocator() const;

protected:
  virtual Uint64 on_read(Byte* _data, Uint64 _size);
  virtual Uint64 on_write(const Byte* _data, Uint64 _size);
  virtual bool on_seek(Uint64 _where);
  virtual bool on_stat(Stat& stat_) const;

private:
  File(void* _impl, const char* _file_name, const char* _mode);

  Memory::Allocator& m_allocator;
  void* m_impl;
  String m_name;
  const char* m_mode;
};

inline constexpr File::File()
  : File{Memory::SystemAllocator::instance()}
{
}

inline constexpr File::File(Memory::Allocator& _allocator)
  : Stream{0}
  , m_allocator{_allocator}
  , m_impl{nullptr}
  , m_name{allocator()}
  , m_mode{nullptr}
{
}

inline File::File(Memory::Allocator& _allocator, const String& _file_name, const char* _mode)
  : File{_allocator, _file_name.data(), _mode}
{
}

inline File::File(const char* _file_name, const char* _mode)
  : File{Memory::SystemAllocator::instance(), _file_name, _mode}
{
}

inline File::File(const String& _file_name, const char* _mode)
  : File{Memory::SystemAllocator::instance(), _file_name, _mode}
{
}

inline File::File(File&& other_)
  : Stream{Utility::move(other_)}
  , m_allocator{other_.m_allocator}
  , m_impl{Utility::exchange(other_.m_impl, nullptr)}
  , m_name{Utility::move(other_.m_name)}
  , m_mode{Utility::exchange(other_.m_mode, nullptr)}
{
}

inline File::~File() {
  close();
}

inline bool File::is_valid() const {
  return m_impl != nullptr;
}

inline File::operator bool() const {
  return is_valid();
}

inline const String& File::name() const & {
  return m_name;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& File::allocator() const {
  return m_allocator;
}

template<typename... Ts>
inline bool File::print(Memory::Allocator& _allocator, const char* _format, Ts&&... _arguments) {
  return print(String::format(_allocator, _format, Utility::forward<Ts>(_arguments)...));
}

template<typename... Ts>
inline bool File::print(const char* _format, Ts&&... _arguments) {
  return print(Memory::SystemAllocator::instance(), _format, Utility::forward<Ts>(_arguments)...);
}

RX_API Optional<Vector<Byte>> read_binary_file(Memory::Allocator& _allocator, const char* _file_name);
RX_API Optional<Vector<Byte>> read_text_file(Memory::Allocator& _allocator, const char* _file_name);

inline Optional<Vector<Byte>> read_binary_file(Memory::Allocator& _allocator, const String& _file_name) {
  return read_binary_file(_allocator, _file_name.data());
}

inline Optional<Vector<Byte>> read_binary_file(const String& _file_name) {
  return read_binary_file(Memory::SystemAllocator::instance(), _file_name);
}

inline Optional<Vector<Byte>> read_binary_file(const char* _file_name) {
  return read_binary_file(Memory::SystemAllocator::instance(), _file_name);
}

inline Optional<Vector<Byte>> read_text_file(Memory::Allocator& _allocator, const String& _file_name) {
  return read_text_file(_allocator, _file_name.data());
}

inline Optional<Vector<Byte>> read_text_file(const String& _file_name) {
  return read_text_file(Memory::SystemAllocator::instance(), _file_name);
}

inline Optional<Vector<Byte>> read_text_file(const char* _file_name) {
  return read_text_file(Memory::SystemAllocator::instance(), _file_name);
}

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_FILE_H

#include <string.h> // strcmp, strerror

#include "rx/core/log.h"
#include "rx/core/assert.h"

#include "rx/core/algorithm/min.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/unreachable.h"

#if defined(RX_PLATFORM_POSIX)
#include <sys/stat.h> // fstat, struct stat
#include <unistd.h> // open, close, pread, pwrite
#include <fcntl.h>
#elif defined(RX_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

RX_LOG("filesystem/file", logger);

namespace Rx::Filesystem {

static inline Uint32 flags_from_mode(const char* _mode) {
  Uint32 flags = 0;

  flags |= Stream::STAT;

  for (const char* ch = _mode; *ch; ch++) {
    switch (*ch) {
    case 'r':
      flags |= Stream::READ;
      break;
    case 'w':
      [[fallthrough]];
    case '+':
      flags |= Stream::WRITE;
      break;
    }
  }

  return flags;
}

#if defined(RX_PLATFORM_POSIX)
union ImplCast { void* impl; int fd; };
#define impl(x) (ImplCast{reinterpret_cast<void*>(x)}.fd)
#elif defined(RX_PLATFORM_WINDOWS)
#define impl(x) (reinterpret_cast<HANDLE>(x))
#endif

#if defined(RX_PLATFORM_POSIX)
static void* open_file([[maybe_unused]] Memory::Allocator& _allocator, const char* _file_name, const char* _mode) {
  // Determine flags to open file by.
  int flags = 0;

  if (strchr(_mode, '+')) {
    flags = O_RDWR;
  } else if (*_mode == 'r') {
    flags = O_RDONLY;
  } else {
    flags = O_WRONLY;
  }

  if (*_mode != 'r') {
    flags |= O_CREAT;
  }

  if (*_mode == 'w') {
    flags |= O_TRUNC;
  } else if (*_mode == 'a') {
    flags |= O_APPEND;
  }

  int fd = open(_file_name, flags, 0666);
  if (fd < 0) {
    return nullptr;
  }

  return reinterpret_cast<void*>(fd);
}

static bool close_file(void* _impl) {
  return close(impl(_impl)) == 0;
}

static bool read_file(void* _impl, Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  const auto result = pread(impl(_impl), _data, _size, _offset);
  if (result == -1) {
    n_bytes_ = 0;
    return false;
  }
  n_bytes_ = static_cast<Size>(result);
  return true;
}

static bool write_file(void* _impl, const Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  const auto result = pwrite(impl(_impl), _data, _size, _offset);
  if (result == -1) {
    n_bytes_ = 0;
    return false;
  }
  n_bytes_ = static_cast<Size>(result);
  return true;
}

static bool stat_file(void* _impl, File::Stat& stat_) {
  struct stat buf;
  if (fstat(impl(_impl), &buf) != -1) {
    stat_.size = buf.st_size;
    return true;
  }
  return false;
}
#elif defined(RX_PLATFORM_WINDOWS)
static void* open_file([[maybe_unused]] Memory::Allocator& _allocator, const char* _file_name, const char* _mode) {
 WideString file_name = String::format(_allocator, "%s", _file_name).to_utf16();

  DWORD dwDesiredAccess = 0;
  DWORD dwShareMode = 0;
  DWORD dwCreationDisposition = 0;
  DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

  // The '+' inside |_mode| indicates R+W
  if (strchr(_mode, '+')) {
    dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
  } else if (*_mode == 'r') {
    // Read-only.
    dwDesiredAccess |= GENERIC_READ;
  } else {
    // Write-only.
    dwDesiredAccess |= GENERIC_WRITE;
  }

  if (*_mode == 'r') {
    // Read only if the file exists.
    dwCreationDisposition = OPEN_EXISTING;
  } else {
    // Write even if the file doesn't exist.
    dwCreationDisposition = OPEN_ALWAYS;
  }

  if (*_mode == 'w') {
    // When writing, truncate to 0 bytes existing files.
    dwCreationDisposition = CREATE_ALWAYS;
  } else if (*_mode == 'a') {
    // Appending.
    dwDesiredAccess |= FILE_APPEND_DATA;
  }

  HANDLE handle = CreateFileW(
    reinterpret_cast<LPCWSTR>(file_name.data()),
    dwDesiredAccess,
    dwShareMode,
    nullptr,
    dwCreationDisposition,
    dwFlagsAndAttributes,
    nullptr);

  return handle != INVALID_HANDLE_VALUE ? handle : nullptr;
}

static bool close_file(void* _impl) {
  return CloseHandle(impl(_impl));
}

static bool read_file(void* _impl, Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  OVERLAPPED overlapped{};
  overlapped.OffsetHigh = static_cast<Uint32>((_offset & 0xFFFFFFFF00000000_u64) >> 32);
  overlapped.Offset = static_cast<Uint32>((_offset & 0xFFFFFFFF_u64));

  long unsigned int n_read_bytes = 0;
  const bool result = ReadFile(
    impl(_impl),
    reinterpret_cast<void*>(_data),
    static_cast<DWORD>(Algorithm::min(_size, 0xFFFFFFFF_z)),
    &n_read_bytes,
    &overlapped);

  n_bytes_ = static_cast<Size>(n_read_bytes);

  if (!result && GetLastError() != ERROR_HANDLE_EOF) {
    n_bytes_ = 0;
    return false;
  }

  return true;
}

static bool write_file(void* _impl, const Byte* _data, Size _size, Uint64 _offset, Size& n_bytes_) {
  OVERLAPPED overlapped{};
  overlapped.OffsetHigh = static_cast<Uint32>((_offset & 0xFFFFFFFF00000000_u64) >> 32);
  overlapped.Offset = static_cast<Uint32>((_offset & 0xFFFFFFFF_u64));

  long unsigned int n_write_bytes = 0;
  const bool result = WriteFile(
    impl(_impl),
    reinterpret_cast<const void*>(_data),
    static_cast<DWORD>(Algorithm::min(_size, 0xFFFFFFFF_z)),
    &n_write_bytes,
    &overlapped);

  n_bytes_ = static_cast<Size>(n_write_bytes);

  return result;
}

static bool stat_file(void* _impl, File::Stat& stat_) {
  BY_HANDLE_FILE_INFORMATION info;
  if (GetFileInformationByHandle(impl(_impl), &info)) {
    // Windows splits size into two 32-bit quanities. Reconstruct as 64-bit value.
    stat_.size = (static_cast<Uint64>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    return true;
  }
  return false;
}
#endif

File::File(Memory::Allocator& _allocator, const char* _file_name, const char* _mode)
  : Stream{flags_from_mode(_mode)}
  , m_allocator{_allocator}
  , m_impl{open_file(m_allocator, _file_name, _mode)}
  , m_name{allocator(), _file_name}
  , m_mode{_mode}
{
}

Uint64 File::on_read(Byte* _data, Uint64 _size, Uint64 _offset) {
  RX_ASSERT(m_impl, "invalid");

  // Process reads in loop since |read_file| can read less than requested.
  Uint64 bytes = 0;
  while (bytes < _size) {
    Size read = 0;
    if (!read_file(m_impl, _data, _size - bytes, _offset + bytes, read) || !read) {
      break;
    }
    bytes += read;
  }
  return bytes;
}

Uint64 File::on_write(const Byte* _data, Uint64 _size, Uint64 _offset) {
  RX_ASSERT(m_impl, "invalid");

  // Process writes in loop since |write_file| can write less than requested.
  Uint64 bytes = 0;
  while (bytes < _size) {
    Size wrote = 0;
    if (!write_file(m_impl, _data, _size - bytes, _offset + bytes, wrote) || !wrote) {
      break;
    }
    bytes += wrote;
  }

  return bytes;
}

bool File::on_stat(Stat& stat_) const {
  RX_ASSERT(m_impl, "invalid");
  return stat_file(m_impl, stat_);
}

bool File::close() {
  if (m_impl && close_file(m_impl)) {
    m_impl = nullptr;
    return true;
  }
  return false;
}

File& File::operator=(File&& file_) {
  RX_ASSERT(&file_ != this, "self assignment");

  close();
  m_impl = Utility::exchange(file_.m_impl, nullptr);
  return *this;
}

bool File::print(String&& contents_) {
  RX_ASSERT(m_impl, "invalid");
  return write(reinterpret_cast<const Byte*>(contents_.data()), contents_.size());
}

bool File::read_line(String& line_) {
  line_.clear();

  for (;;) {
    char buffer[1024];
    const auto n_bytes = read(reinterpret_cast<Byte*>(buffer), sizeof buffer);

    // Check for EOS.
    if (is_eos()) {
      return !line_.is_empty();
    }

    // Search the buffer for one of '\r' or '\n'. Count the length of the string
    // up to such a character.
    Uint64 length = 0;
    Size new_line_length = 2;
    for (Uint64 i = 0; i < n_bytes; i++) {
      const int ch = buffer[i];
      // Handles lines terminated by \r, \r\n, and \n.
      if (ch == '\r' || ch == '\n') {
        if (ch == '\r') {
          new_line_length = (i + 1 < n_bytes && buffer[i + 1] == '\n' ? 2 : 1);
        } else {
          new_line_length = 1;
        }
        break;
      }
      length++;
    }

    // Line does not terminate inside |buffer|.
    if (length == n_bytes) {
      line_.append(buffer, n_bytes);
    } else {
      // The line terminates inside the buffer.
      const auto where = static_cast<Sint64>(n_bytes - (length + new_line_length));

      // Seek back in the stream to right after the new line.
      if (!seek(-where, Whence::CURRENT)) {
        return false;
      }

      line_.append(buffer, length);
      return true;
    }
  }

  RX_HINT_UNREACHABLE();
}

Optional<Vector<Byte>> read_binary_file(Memory::Allocator& _allocator, const char* _file_name) {
  if (File open_file{_file_name, "rb"}) {
    return read_binary_stream(_allocator, &open_file);
  }

  logger->error("failed to open file '%s' [%s]", _file_name);
  return nullopt;
}

Optional<Vector<Byte>> read_text_file(Memory::Allocator& _allocator, const char* _file_name) {
  if (File open_file{_file_name, "rb"}) {
    return read_text_stream(_allocator, &open_file);
  }

  logger->error("failed to open file '%s'", _file_name);
  return nullopt;
}

} // namespace rx::filesystem

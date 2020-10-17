#include <stdio.h> // FILE, f{open,close,read,write,seek}
#include <errno.h> // errno
#include <string.h> // strcmp, strerror
#include <sys/stat.h> // fstat, struct stat

#include "rx/core/log.h"
#include "rx/core/assert.h"
#include "rx/core/config.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/unreachable.h"

RX_LOG("filesystem/file", logger);

namespace Rx::Filesystem {

static inline Uint32 flags_from_mode(const char* _mode) {
  Uint32 flags = 0;

  flags |= Stream::SEEK;
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

static void* open_file([[maybe_unused]] Memory::Allocator& _allocator, const char* _file_name, const char* _mode) {
  FILE* fp = nullptr;

#if defined(RX_PLATFORM_WINDOWS)
  // Convert |_file_name| to UTF-16.
  const WideString file_name = String(_allocator, _file_name).to_utf16();

  // Convert the mode string to a wide char version. The mode string is in ascii
  // so there's no conversion necessary other than extending the Type size.
  wchar_t mode_buffer[8];
  wchar_t *mode = mode_buffer;
  for (const char* ch = _mode; *ch; ch++) {
    *mode++ = static_cast<wchar_t>(*ch);
  }
  // Null-terminate mode.
  *mode++ = L'\0';

  // Utilize _wfopen on Windows so we can open files with UNICODE names.
  fp = _wfopen(reinterpret_cast<const wchar_t*>(file_name.data()), mode_buffer);
#else
  fp = fopen(_file_name, _mode);
#endif

  if (fp) {
    // Disable buffering.
    setvbuf(fp, nullptr, _IONBF, 0 );
    return static_cast<void*>(fp);
  }

  return nullptr;
}

File::File(Memory::Allocator& _allocator, const char* _file_name, const char* _mode)
  : Stream{flags_from_mode(_mode)}
  , m_allocator{_allocator}
  , m_impl{open_file(m_allocator, _file_name, _mode)}
  , m_name{allocator(), _file_name}
  , m_mode{_mode}
{
}

Uint64 File::on_read(Byte* _data, Uint64 _size) {
  RX_ASSERT(m_impl, "invalid");
  return fread(_data, 1, _size, static_cast<FILE*>(m_impl));
}

Uint64 File::on_write(const Byte* _data, Uint64 _size) {
  RX_ASSERT(m_impl, "invalid");
  return fwrite(_data, 1, _size, static_cast<FILE*>(m_impl));
}

bool File::on_seek(Uint64 _where) {
  RX_ASSERT(m_impl, "invalid");

  const auto fp = static_cast<FILE*>(m_impl);
  const auto where = static_cast<long>(_where);

  return fseek(fp, where, SEEK_SET) == 0;
}

bool File::on_stat(Stat& stat_) const {
  RX_ASSERT(m_impl, "invalid");

  const auto fp = static_cast<FILE*>(m_impl);
#if defined(RX_PLATFORM_WINDOWS)
  struct _stat buf;
  const auto fd = _fileno(fp);
  const auto status = _fstat(fd, &buf);
#else
  struct stat buf;
  const auto fd = fileno(fp);
  const auto status = fstat(fd, &buf);
#endif

  if (status != -1) {
    stat_.size = buf.st_size;
    return true;
  }

  return false;
}

bool File::close() {
  if (m_impl) {
    fclose(static_cast<FILE*>(m_impl));
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
  RX_ASSERT(strcmp(m_mode, "w") == 0, "cannot print with mode '%s'", m_mode);
  return fprintf(static_cast<FILE*>(m_impl), "%s", contents_.data()) > 0;
}

bool File::read_line(String& line_) {
  auto* fp = static_cast<FILE*>(m_impl);

  line_.clear();
  for (;;) {
    char buffer[4096];
    if (!fgets(buffer, sizeof buffer, fp)) {
      if (feof(fp)) {
        return !line_.is_empty();
      }

      return false;
    }

    Size length{strlen(buffer)};

    if (length && buffer[length - 1] == '\n') {
      length--;
    }

    if (length && buffer[length - 1] == '\r') {
      length--;
    }

    line_.append(buffer, length);

    if (length < sizeof buffer - 1) {
      return true;
    }
  }

  return false;
}

Optional<Vector<Byte>> read_binary_file(Memory::Allocator& _allocator, const char* _file_name) {
  if (File open_file{_file_name, "rb"}) {
    return read_binary_stream(_allocator, &open_file);
  }

  logger->error("failed to open file '%s' [%s]", _file_name,
    strerror(errno));

  return nullopt;
}

Optional<Vector<Byte>> read_text_file(Memory::Allocator& _allocator, const char* _file_name) {
  if (File open_file{_file_name, "rb"}) {
    return read_text_stream(_allocator, &open_file);
  }

  logger->error("failed to open file '%s' [%s]", _file_name,
    strerror(errno));

  return nullopt;
}

} // namespace rx::filesystem

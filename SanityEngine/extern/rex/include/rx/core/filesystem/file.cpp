#include <stdio.h> // FILE, f{open,close,read,write,seek,flush}
#include <errno.h> // errno
#include <string.h> // strcmp, strerror

#include "rx/core/log.h"
#include "rx/core/assert.h"
#include "rx/core/config.h"

#include "rx/core/filesystem/file.h"

#include "rx/core/hints/unlikely.h"
#include "rx/core/hints/unreachable.h"

RX_LOG("filesystem/file", logger);

namespace Rx::Filesystem {

Uint32 File::flags_from_mode(const char* _mode) {
  Uint32 flags = 0;

  // These few flags are true regardless of the mode.
  flags |= Stream::k_tell;
  flags |= Stream::k_seek;

  for (const char* ch = _mode; *ch; ch++) {
    switch (*ch) {
    case 'r':
      flags |= Stream::k_read;
      break;
    case 'w':
      [[fallthrough]];
    case '+':
      flags |= Stream::k_write;
      flags |= Stream::k_flush;
      break;
    }
  }

  return flags;
}

#if defined(RX_PLATFORM_WINDOWS)
File::File(Memory::Allocator& _allocator, const char* _file_name, const char* _mode)
  : Stream{flags_from_mode(_mode)}
  , m_allocator{_allocator}
  , m_impl{nullptr}
  , m_name{allocator(), _file_name}
  , m_mode{_mode}
{
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
  m_impl = static_cast<void*>(
    _wfopen(reinterpret_cast<const wchar_t*>(file_name.data()), mode_buffer));
}
#else
File::File(Memory::Allocator& _allocator, const char* _file_name, const char* _mode)
  : Stream{flags_from_mode(_mode)}
  , m_allocator{_allocator}
  , m_impl{static_cast<void*>(fopen(_file_name, _mode))}
  , m_name{allocator(), _file_name}
  , m_mode{_mode}
{
}
#endif

Uint64 File::on_read(Byte* _data, Uint64 _size) {
  RX_ASSERT(m_impl, "invalid");
  return fread(_data, 1, _size, static_cast<FILE*>(m_impl));
}

Uint64 File::on_write(const Byte* _data, Uint64 _size) {
  RX_ASSERT(m_impl, "invalid");
  return fwrite(_data, 1, _size, static_cast<FILE*>(m_impl));
}

bool File::on_seek(Sint64 _where, Whence _whence) {
  RX_ASSERT(m_impl, "invalid");

  const auto fp = static_cast<FILE*>(m_impl);
  const auto where = static_cast<long>(_where);

  switch (_whence) {
  case Whence::k_set:
    return fseek(fp, where, SEEK_SET) == 0;
  case Whence::k_current:
    return fseek(fp, where, SEEK_CUR) == 0;
  case Whence::k_end:
    return fseek(fp, where, SEEK_END) == 0;
  }

  RX_HINT_UNREACHABLE();
}

Uint64 File::on_tell() {
  RX_ASSERT(m_impl, "invalid");
  const auto fp = static_cast<FILE*>(m_impl);
  return ftell(fp);
}

bool File::on_flush() {
  RX_ASSERT(m_impl, "invalid");
  return fflush(static_cast<FILE*>(m_impl)) == 0;
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

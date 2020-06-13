#include "rx/core/filesystem/path_resolver.h"

namespace Rx::Filesystem {

bool PathResolver::Stack::push() {
  if (size >= sizeof data / sizeof *data) {
    return false;
  }

  data[size++] = next;
  next = -1_z;

  return true;
}

Size PathResolver::Stack::pop() {
  const auto result = data[size - 1];

  // Clear out what ever was queued for the next push.
  next = -1_z;

  // Don't allow popping off the root element.
  if (size > 1) {
    size--;
  }

  return result;
}

bool PathResolver::append(const char* _path) {
  // Always have the root character in the path at the beginning.
  if (m_data.is_empty() && !m_data.push_back('/')) {
    return false;
  }

  // When appending be sure we have a path separator.
  if ((m_data.is_empty() || m_data.last() != '/') && !push('/')) {
    return false;
  }

  for (const char* ch = _path; *ch; ch++) {
    if (!push(*ch)) {
      return false;
    }
  }

  return true;
}

bool PathResolver::push(int _ch) {
  if (m_data.last() == '\0') {
    // Path is terminated.
    return false;
  }

  switch (_ch) {
  case '/':
    [[fallthrough]];
  case '\\':
    switch (m_dots) {
    case 0: // "/"
      if (m_data.last() == '/') {
        // Treat constructions like "foo//bar" as "foo/bar".
        break;
      }
      if (RX_HINT_UNLIKELY(!m_data.push_back('/'))) {
        // Out of memory.
        return false;
      }
      if (m_stack.next != -1_z) {
        if (!m_stack.push()) {
          // Path is too deep.
          return false;
        }
      } else {
        m_stack.next = m_data.size();
      }
      break;
    case 1: // "./" or ".\\"
      m_data.pop_back(); // Unget '.'
      if (m_data.last() != '/') {
        // Use of "./" or ".\\" without path separator before it.
        return false;
      }
      break;
    case 2: // "../" or "..\\"
      m_data.pop_back(); // Unget '.'
      m_data.pop_back(); // Unget '.'
      // m_data.pop_back();
      if (m_data.last() != '/') {
        // Use of "../" or "..\\" without path separator before it.
        return false;
      }
      // This resize cannot fail because it only makes |m_data| smaller.
      m_data.resize(m_stack.pop());
      break;
    }
    m_dots = 0;
    break;
  case '.':
    // Append '.' since it might be part of a path or file name.
    if (RX_HINT_UNLIKELY(!m_data.push_back('.'))) {
      // Out of memory.
      return false;
    }
    if (m_dots == 2) {
      // Too many dots in path.
      return false;
    }
    m_dots++;
    break;
  // These are invalid characters in a path name on Windows. We make them
  // invalid on all platforms because we don't want to encourage platform
  // inconsistencies.
  // https://support.microsoft.com/en-us/help/177506
  case '<': [[fallthrough]]; // Redirection.
  case '>': [[fallthrough]]; // Redirection.
  case ':': [[fallthrough]]; // Drive designator.
  case '"': [[fallthrough]]; // To allow spaces in parameters.
  case '|': [[fallthrough]]; // Pipe.
  case '?': [[fallthrough]]; // Wildcard.
  case '*': [[fallthrough]]; // Wildcard.
  case '^':                  // Invalid on FAT file systems.
    return false;
  case '\0':
    if (m_dots != 0) {
      // Unfinished dots in path.
      return false;
    }
    if (RX_HINT_UNLIKELY(!m_data.push_back('\0'))) {
      // Out of memory.
      return false;
    }
    if (m_stack.next != -1_z && !m_stack.push()) {
      // Path is too deep.
      return false;
    }

    m_stack.next = m_data.size();
    if (!m_stack.push()) {
      // Path is too deep.
      return false;
    }
    break;
  default:
    if (m_dots == 2) {
      // Expected '/' or '\\' after "..".
      return false;
    }
    if (RX_HINT_UNLIKELY(!m_data.push_back(_ch))) {
      // Out of memory.
      return false;
    }
    m_dots = 0;
    break;
  }
  return true;
}

} // namespace rx::filesystem

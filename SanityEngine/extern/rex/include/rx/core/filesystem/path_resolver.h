#ifndef RX_CORE_FILESYSTEM_PATH_RESOLVER_H
#define RX_CORE_FILESYSTEM_PATH_RESOLVER_H
#include "rx/core/vector.h"
#include "rx/core/string.h"

namespace Rx::Filesystem {

// # Incremental Path Resolver
//
// An incremental path resolver that generates a fully qualified path URI for
// the virtual file system.
//
// You can append to the path sub paths or file names and it will form a fully
// qualified path name. Alternatively, you can push individual characters into
// the resolver and as it recieves them it'll translate the path accordingly.
//
// When you're ready to use the qualified path, call finalize().
struct RX_API PathResolver {
  constexpr PathResolver();
  constexpr PathResolver(Memory::Allocator& _allocator);

  [[nodiscard]] bool append(const String& _path);
  [[nodiscard]] bool append(const char* _path);
  [[nodiscard]] bool push(int _ch);

  const char* path() const;

  String operator[](Size _index) {
    const auto beg = _index ? m_stack.data[_index - 1] : 0;
    const auto end = m_stack.data[_index];
    return {m_data.allocator(), m_data.data() + beg, m_data.data() + end};
  }

  Size parts() const {
    return m_stack.size;
  }

private:
  struct Stack {
    constexpr Stack();

    bool push();
    Size pop();

    union {
      Size head;
      Size data[255];
    };
    Size size;
    Size next;
  };

  [[nodiscard]] bool reserve_more(Size _size);

  Vector<char> m_data;
  Stack m_stack;
  Size m_dots;
};

inline constexpr PathResolver::Stack::Stack()
  : head{1}
  , size{1}
  , next{-1_z}
{
}

inline constexpr PathResolver::PathResolver()
  : PathResolver{Memory::SystemAllocator::instance()}
{
}

inline constexpr PathResolver::PathResolver(Memory::Allocator& _allocator)
  : m_data{_allocator}
  , m_dots{0}
{
}

inline const char* PathResolver::path() const {
  return m_data.data();
}

inline bool PathResolver::append(const String& _path) {
  return reserve_more(_path.size()) && append(_path.data());
}

inline bool PathResolver::reserve_more(Size _size) {
  return m_data.reserve(m_data.capacity() + _size);
}

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_PATH_RESOLVER_H

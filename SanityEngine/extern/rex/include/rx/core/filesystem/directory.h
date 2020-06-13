#ifndef RX_CORE_FILESYSTEM_DIRECTORY_H
#define RX_CORE_FILESYSTEM_DIRECTORY_H
#include "rx/core/string.h"
#include "rx/core/function.h"

#include "rx/core/concepts/no_copy.h"

#include "rx/core/hints/empty_bases.h"

namespace Rx::Filesystem {

struct RX_HINT_EMPTY_BASES Directory
  : Concepts::NoCopy
{
  Directory(Memory::Allocator& _allocator, const char* _path);
  Directory(Memory::Allocator& _allocator, const String& _path);
  Directory(Memory::Allocator& _allocator, String&& path_);
  Directory(const char* _path);
  Directory(const String& _path);
  Directory(String&& path_);
  ~Directory();

  operator bool() const;

  struct Item {
    enum class Type : Uint8 {
      k_file,
      k_directory
    };

    bool is_file() const;
    bool is_directory() const;
    const String& name() const;
    String&& name();

  private:
    friend struct Directory;

    Item(String&& name_, Type _type);

    String m_name;
    Type m_type;
  };

  // enumerate directory with |_function| being called for each item
  // NOTE: does not consider hidden files, symbolic links, block devices, or ..
  void each(Function<void(Item&&)>&& _function);

  const String& path() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  Memory::Allocator& m_allocator;
  String m_path;
  void* m_impl;
};

inline Directory::Directory(Memory::Allocator& _allocator, const char* _path)
  : Directory{_allocator, String{_allocator, _path}}
{
}

inline Directory::Directory(Memory::Allocator& _allocator, const String& _path)
  : Directory{_allocator, String{_allocator, _path}}
{
}

inline Directory::Directory(const char* _path)
  : Directory{Memory::SystemAllocator::instance(), _path}
{
}

inline Directory::Directory(const String& _path)
  : Directory{Memory::SystemAllocator::instance(), _path}
{
}

inline Directory::Directory(String&& path_)
  : Directory{Memory::SystemAllocator::instance(), Utility::move(path_)}
{
}

RX_HINT_FORCE_INLINE Directory::operator bool() const {
  return m_impl;
}

RX_HINT_FORCE_INLINE const String& Directory::path() const & {
  return m_path;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Directory::allocator() const {
  return m_allocator;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_file() const {
  return m_type == Type::k_file;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_directory() const {
  return m_type == Type::k_directory;
}

RX_HINT_FORCE_INLINE const String& Directory::Item::name() const {
  return m_name;
}

RX_HINT_FORCE_INLINE String&& Directory::Item::name() {
  return Utility::move(m_name);
}

RX_HINT_FORCE_INLINE Directory::Item::Item(String&& name_, Type _type)
  : m_name{Utility::move(name_)}
  , m_type{_type}
{
}

bool create_directory(const String& _path);

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_DIRECTORY_H

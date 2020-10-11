#ifndef RX_CORE_FILESYSTEM_DIRECTORY_H
#define RX_CORE_FILESYSTEM_DIRECTORY_H
#include "rx/core/string.h"
#include "rx/core/function.h"
#include "rx/core/optional.h"

namespace Rx::Filesystem {

struct RX_API Directory {
  RX_MARK_NO_COPY(Directory);

  Directory(Memory::Allocator& _allocator, const char* _path);
  Directory(Memory::Allocator& _allocator, const String& _path);
  Directory(Memory::Allocator& _allocator, String&& path_);
  Directory(const char* _path);
  Directory(const String& _path);
  Directory(String&& path_);
  Directory(Directory&& directory_);
  ~Directory();

  operator bool() const;

  struct Item {
    RX_MARK_NO_COPY(Item);
    RX_MARK_NO_MOVE(Item);

    enum class Type : Uint8 {
      k_file,
      k_directory
    };

    bool is_file() const;
    bool is_directory() const;
    const String& name() const;
    String&& name();

    const Directory& directory() const &;

    Optional<Directory> as_directory() const;

  private:
    friend struct Directory;

    Item(const Directory* _directory, String&& name_, Type _type);

    const Directory* m_directory;
    String m_name;
    Type m_type;
  };

  // enumerate directory with |_function| being called for each item
  // NOTE: does not consider hidden files, symbolic links, block devices, or ..
  void each(Function<void(Item&&)>&& _function);

  const String& path() const &;

  constexpr Memory::Allocator& allocator() const;

private:
  Memory::Allocator* m_allocator;
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

inline Directory::Directory(Directory&& directory_)
  : m_allocator{directory_.m_allocator}
  , m_path{Utility::move(directory_.m_path)}
  , m_impl{Utility::exchange(directory_.m_impl, nullptr)}
{
}

RX_HINT_FORCE_INLINE Directory::operator bool() const {
  return m_impl;
}

RX_HINT_FORCE_INLINE const String& Directory::path() const & {
  return m_path;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Directory::allocator() const {
  return *m_allocator;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_file() const {
  return m_type == Type::k_file;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_directory() const {
  return m_type == Type::k_directory;
}

RX_HINT_FORCE_INLINE const Directory& Directory::Item::directory() const & {
  return *m_directory;
}

RX_HINT_FORCE_INLINE const String& Directory::Item::name() const {
  return m_name;
}

RX_HINT_FORCE_INLINE String&& Directory::Item::name() {
  return Utility::move(m_name);
}

RX_HINT_FORCE_INLINE Directory::Item::Item(const Directory* _directory, String&& name_, Type _type)
  : m_directory{_directory}
  , m_name{Utility::move(name_)}
  , m_type{_type}
{
}

bool create_directory(const String& _path);

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_DIRECTORY_H

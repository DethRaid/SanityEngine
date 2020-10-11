#ifndef RX_CORE_GLOBAL_H
#define RX_CORE_GLOBAL_H
#include "rx/core/intrusive_compressed_list.h"
#include "rx/core/intrusive_list.h"
#include "rx/core/uninitialized.h"
#include "rx/core/tagged_ptr.h"

namespace Rx {

// 32-bit: 32 bytes
// 64-bit: 64 bytes
struct RX_API alignas(Memory::Allocator::ALIGNMENT) GlobalNode {
  template<typename T, typename... Ts>
  GlobalNode(const char* _group, const char* _name, Uninitialized<T>& _storage, Ts&&... _arguments);

  void init();
  void fini();

  template<typename... Ts>
  void init(Ts&&... _arguments);

  const char* name() const;

  Byte* data();

  template<typename T>
  T* cast();

  template<typename T>
  const T* cast() const;

private:
  friend struct Globals;
  friend struct GlobalGroup;

  template<typename T>
  void validate_cast_for() const;

  template<typename F, typename... Rs>
  struct Arguments : Arguments<Rs...> {
    constexpr Arguments(const F& _first, Rs&&... _rest)
      : Arguments<Rs...>(Utility::forward<Rs>(_rest)...)
      , first{_first}
    {
    }
    F first;
  };
  template<typename F>
  struct Arguments<F> {
    constexpr Arguments(const F& _first)
      : first{_first}
    {
    }
    F first;
  };

  template<Size I, typename F, typename... Rs>
  struct ReadArgument {
    static auto value(const Arguments<F, Rs...>* _arguments) {
      return ReadArgument<I - 1, Rs...>::value(_arguments);
    }
  };
  template<typename F, typename... Rs>
  struct ReadArgument<0, F, Rs...> {
    static F value(const Arguments<F, Rs...>* _arguments) {
      return _arguments->first;
    }
  };
  template<Size I, typename F, typename... Rs>
  static auto argument(const Arguments<F, Rs...>* _arguments) {
    return ReadArgument<I, F, Rs...>::value(_arguments);
  }

  template<Size...>
  struct UnpackSequence {};
  template<Size N, Size... Ns>
  struct UnpackArguments
    : UnpackArguments<N - 1, N - 1, Ns...>
  {
  };
  template<Size... Ns>
  struct UnpackArguments<0, Ns...> {
    using Type = UnpackSequence<Ns...>;
  };

  template<typename... Ts>
  static void construct_arguments(Byte* _argument_store, Ts... _arguments) {
    Utility::construct<Arguments<Ts...>>(_argument_store, Utility::forward<Ts>(_arguments)...);
  }

  template<typename T, typename... Ts, Size... Ns>
  static void construct_global(UnpackSequence<Ns...>, Byte* _storage,
    [[maybe_unused]] Byte* _argument_store)
  {
    Utility::construct<T>(_storage,
                          argument<Ns>(reinterpret_cast<Arguments<Ts...>*>(_argument_store))...);
  }

  // Compression technique. Only need to store a single function pointer.
  enum class StorageMode {
    INIT,
    FINI,
    TRAITS
  };

  template<typename T, typename... Ts>
  static Uint64 storage_dispatch(StorageMode _mode,
    [[maybe_unused]] Byte* _global_store, [[maybe_unused]] Byte* _argument_store)
  {
    switch (_mode) {
    case StorageMode::INIT:
      using Unpack = typename UnpackArguments<sizeof...(Ts)>::Type;
      construct_global<T, Ts...>(Unpack{}, _global_store, _argument_store);
      break;
    case StorageMode::FINI:
      if (_global_store) {
        Utility::destruct<T>(_global_store);
      }
      if constexpr(sizeof...(Ts) != 0) {
        if (_argument_store) {
          Utility::destruct<Arguments<Ts...>>(_argument_store);
        }
      }
      break;
    case StorageMode::TRAITS:
      return (Uint64(sizeof(T)) << 32_u64) | alignof(T);
    }
    return 0;
  }

  Byte* allocate(Size _size);
  void deallocate(Byte* _data);

  // Flag is set in |m_argument_store|.
  enum : Byte {
    INITIALIZED = 1 << 0,
    ARGUMENTS   = 1 << 1
  };

  TaggedPtr<Byte> m_argument_store;

  IntrusiveCompressedList::Node m_grouped;
  IntrusiveCompressedList::Node m_ungrouped;

  IntrusiveList::Node m_initialized;

  const char* m_group;
  const char* m_name;

  Uint64 (*m_storage_dispatch)(StorageMode _mode, Byte* _global_store,
    Byte* _argument_store);
};

// 32-bit: 32 + sizeof(T) bytes
// 64-bit: 64 + sizeof(T) bytes
template<typename T>
struct RX_API Global {
  template<typename... Ts>
  Global(const char* _group, const char* _name, Ts&&... _arguments);

  void init();
  void fini();

  template<typename... Ts>
  void init(Ts&&... _arguments);

  constexpr const char* name() const;

  constexpr T* operator&();
  constexpr const T* operator&() const;

  constexpr T& operator*();
  constexpr const T& operator*() const;

  constexpr T* operator->();
  constexpr const T* operator->() const;

  constexpr T* data();
  constexpr const T* data() const;

private:
  mutable GlobalNode m_node;
  Uninitialized<T> m_global_store;
};

// 32-bit: 20 bytes
// 64-bit: 40 bytes
struct RX_API GlobalGroup {
  GlobalGroup(const char* _name);

  constexpr const char* name() const;

  GlobalNode* find(const char* _name);

  void init();
  void fini();

  template<typename F>
  void each(F&& _function);

private:
  friend struct Globals;
  friend struct GlobalNode;

  const char* m_name;

  // Nodes for this group. This is constructed after a call to |Globals::link|.
  IntrusiveCompressedList m_list;

  // Link for global linked-list of groups in |globals|.
  IntrusiveCompressedList::Node m_link;
};

struct Globals {
  static GlobalGroup* find(const char* _name);

  // Goes over global linked-list of groups, adding nodes to the group
  // if the |GlobalNode::m_group| matches the group name.
  static void link();

  static void init();
  static void fini();

private:
  friend struct GlobalNode;
  friend struct GlobalGroup;

  static void link(GlobalNode* _node);
  static void link(GlobalGroup* _group);

  // Global linked-list of groups.
  static inline IntrusiveCompressedList s_group_list;

  // Global linked-list of ungrouped nodes.
  static inline IntrusiveCompressedList s_node_list;

  // Global linked-list of initialized nodes.
  static inline IntrusiveList s_initialized_list;
};

// GlobalNode
template<typename T, typename... Ts>
inline GlobalNode::GlobalNode(const char* _group, const char* _name,
                              Uninitialized<T>& _global_store, Ts&&... _arguments)
  : m_group{_group ? _group : "system"}
  , m_name{_name}
  , m_storage_dispatch{storage_dispatch<T, Ts...>}
{
  RX_ASSERT(reinterpret_cast<UintPtr>(&_global_store)
    == reinterpret_cast<UintPtr>(data()), "misalignment");

  if constexpr (sizeof...(Ts) != 0) {
    Byte* argument_store = allocate(sizeof(Arguments<Ts...>));
    m_argument_store = {argument_store, ARGUMENTS};
    construct_arguments(m_argument_store.as_ptr(), Utility::forward<Ts>(_arguments)...);
  }

  Globals::link(this);
}

template<typename... Ts>
inline void GlobalNode::init(Ts&&... _arguments) {
  static_assert(sizeof...(Ts) != 0,
    "use void init() for default construction");

  // Call destructor on existing arguments.
  if (m_argument_store.as_tag() & ARGUMENTS) {
    m_storage_dispatch(StorageMode::FINI, nullptr, m_argument_store.as_ptr());
  }

  // Construct the new arguments in that memory.
  construct_arguments(m_argument_store.as_ptr(), Utility::forward<Ts>(_arguments)...);

  init();
}

inline const char* GlobalNode::name() const {
  return m_name;
}

inline Byte* GlobalNode::data() {
  // The layout of a Global<T> is such that the storage for it is right after
  // the node. That node is |this|, this puts the storage one-past |this|.
  return reinterpret_cast<Byte*>(this + 1);
}

template<typename T>
inline T* GlobalNode::cast() {
  validate_cast_for<T>();
  return reinterpret_cast<T*>(data());
}

template<typename T>
void GlobalNode::validate_cast_for() const {
  const auto traits = m_storage_dispatch(StorageMode::TRAITS, nullptr, nullptr);
  RX_ASSERT(sizeof(T) == ((traits >> 32) & 0xFFFFFFFF_u32), "invalid size");
  RX_ASSERT(alignof(T) == (traits & 0xFFFFFFFF_u32), "invalid allignment");
}

// GlobalGroup
inline GlobalGroup::GlobalGroup(const char* _name)
  : m_name{_name}
{
  Globals::link(this);
}

inline constexpr const char* GlobalGroup::name() const {
  return m_name;
}

template<typename F>
inline void GlobalGroup::each(F&& _function) {
  for (auto node = m_list.enumerate_head(&GlobalNode::m_grouped); node; node.next()) {
    _function(node.data());
  }
}

// Global
template<typename T>
template<typename... Ts>
inline Global<T>::Global(const char* _group, const char* _name, Ts&&... _arguments)
  : m_node{_group, _name, m_global_store, Utility::forward<Ts>(_arguments)...}
{
}

template<typename T>
inline void Global<T>::init() {
  m_node.init();
}

template<typename T>
inline void Global<T>::fini() {
  m_node.fini();
}

template<typename T>
template<typename... Ts>
inline void Global<T>::init(Ts&&... _arguments) {
  m_node.init(Utility::forward<Ts>(_arguments)...);
}

template<typename T>
inline constexpr const char* Global<T>::name() const {
  return m_node.name();
}

template<typename T>
inline constexpr T* Global<T>::operator&() {
  return data();
}

template<typename T>
inline constexpr const T* Global<T>::operator&() const {
  return data();
}

template<typename T>
inline constexpr T& Global<T>::operator*() {
  return *data();
}

template<typename T>
inline constexpr const T& Global<T>::operator*() const {
  return *data();
}

template<typename T>
constexpr T* Global<T>::operator->() {
  return data();
}

template<typename T>
constexpr const T* Global<T>::operator->() const {
  return data();
}

template<typename T>
inline constexpr T* Global<T>::data() {
  m_node.init();
  return m_global_store.data();
}

template<typename T>
inline constexpr const T* Global<T>::data() const {
  m_node.init();
  return m_global_store.data();
}

} // namespace Rx

#endif // RX_CORE_GLOBAL_H

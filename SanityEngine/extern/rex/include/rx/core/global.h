#ifndef RX_CORE_GLOBAL_H
#define RX_CORE_GLOBAL_H
#include "rx/core/tagged_ptr.h"
#include "rx/core/intrusive_xor_list.h"
#include "rx/core/memory/uninitialized_storage.h"

namespace Rx {

// 32-bit: 24 bytes
// 64-bit: 48 bytes
struct alignas(Memory::Allocator::k_alignment) GlobalNode {
  template<typename T, typename... Ts>
  GlobalNode(const char* _group, const char* _name,
             Memory::UninitializedStorage<T>& _storage, Ts&&... _arguments);

  void init();
  void fini();

  template<typename... Ts>
  void init(Ts&&... _arguments);

  const char* name() const;

  Byte* data();
  const Byte* data() const;

  template<typename T>
  T* cast();

  template<typename T>
  const T* cast() const;

private:
  friend struct Globals;
  friend struct GlobalGroup;

  void init_global();
  void fini_global();

  template<typename T>
  void validate_cast_for() const;

  template<typename F, typename... Rs>
  struct Arguments : Arguments<Rs...> {
    constexpr Arguments(F&& _first, Rs&&... _rest)
      : Arguments<Rs...>(Utility::forward<Rs>(_rest)...)
      , first{Utility::forward<F>(_first)}
    {
    }
    F first;
  };
  template<typename F>
  struct Arguments<F> {
    constexpr Arguments(F&& _first)
      : first{Utility::forward<F>(_first)}
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
    Utility::construct<Arguments<Ts...>>(_argument_store,
                                         Utility::forward<Ts>(_arguments)...);
  }

  template<typename T, typename... Ts, Size... Ns>
  static void construct_global(UnpackSequence<Ns...>, Byte* _storage,
    [[maybe_unused]] Byte* _argument_store)
  {
    Utility::construct<T>(_storage,
                          argument<Ns>(reinterpret_cast<Arguments<Ts...>*>(_argument_store))...);
  }

  // Combine multiple operations into a single function so we only have to store
  // a single function pointer rather than multiple.
  enum class StorageMode {
    k_init_global,
    k_fini_global,
    k_traits_global,
    k_fini_arguments
  };

  template<typename T, typename... Ts>
  static Uint64 storage_dispatch(StorageMode _mode,
    [[maybe_unused]] Byte* _global_store, [[maybe_unused]] Byte* _argument_store)
  {
    switch (_mode) {
    case StorageMode::k_init_global:
      using Unpack = typename UnpackArguments<sizeof...(Ts)>::Type;
      construct_global<T, Ts...>(Unpack{}, _global_store, _argument_store);
      break;
    case StorageMode::k_fini_global:
      Utility::destruct<T>(_global_store);
      break;
    case StorageMode::k_fini_arguments:
      if constexpr(sizeof...(Ts) != 0) {
        Utility::destruct<Arguments<Ts...>>(_argument_store);
      }
      break;
    case StorageMode::k_traits_global:
      return (sizeof(T) << 32_u64) | alignof(T);
    }
    return 0;
  }

  static Byte* reallocate_arguments(Byte* _existing, Size _size);

  // Stored in tag bits of |m_argument_store|.
  enum : Byte {
    k_enabled     = 1 << 0,
    k_initialized = 1 << 1,
    k_arguments   = 1 << 2
  };

  TaggedPtr<Byte> m_argument_store;

  intrusive_xor_list::Node m_grouped;
  intrusive_xor_list::Node m_ungrouped;

  const char* m_group;
  const char* m_name;

  Uint64 (*m_storage_dispatch)(StorageMode _mode, Byte* _global_store,
    Byte* _argument_store);
};

// 32-bit: 24 + sizeof(T) bytes
// 64-bit: 48 + sizeof(T) bytes
template<typename T>
struct Global {
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
  GlobalNode m_node;
  Memory::UninitializedStorage<T> m_global_store;
};

// 32-bit: 20 bytes
// 64-bit: 40 bytes
struct GlobalGroup {
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

  void init_global();
  void fini_global();

  const char* m_name;

  // Nodes for this group. This is constructed after a call to |globals::link|.
  intrusive_xor_list m_list;

  // Link for global linked-list of groups in |globals|.
  intrusive_xor_list::Node m_link;
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
  static inline intrusive_xor_list s_group_list;

  // Global linked-list of ungrouped nodes.
  static inline intrusive_xor_list s_node_list;
};

// GlobalNode
template<typename T, typename... Ts>
inline GlobalNode::GlobalNode(const char* _group, const char* _name,
                              Memory::UninitializedStorage<T>& _global_store, Ts&&... _arguments)
  : m_group{_group ? _group : "system"}
  , m_name{_name}
  , m_storage_dispatch{storage_dispatch<T, Ts...>}
{
  RX_ASSERT(reinterpret_cast<UintPtr>(&_global_store)
    == reinterpret_cast<UintPtr>(data()), "misalignment");

  if constexpr (sizeof...(Ts) != 0) {
    Byte* argument_store = reallocate_arguments(nullptr, sizeof(Arguments<Ts...>));
    m_argument_store = {argument_store, k_enabled | k_arguments};
    construct_arguments(m_argument_store.as_ptr(), Utility::forward<Ts>(_arguments)...);
  } else {
    m_argument_store = {nullptr, k_enabled};
  }

  Globals::link(this);
}

template<typename... Ts>
inline void GlobalNode::init(Ts&&... _arguments) {
  static_assert(sizeof...(Ts) != 0,
    "use void init() for default construction");

  auto argument_store = m_argument_store.as_ptr();
  if (m_argument_store.as_tag() & k_arguments) {
    m_storage_dispatch(StorageMode::k_fini_arguments, data(), argument_store);
  }

  construct_arguments(argument_store, Utility::forward<Ts>(_arguments)...);

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

inline const Byte* GlobalNode::data() const {
  return reinterpret_cast<const Byte*>(this + 1);
}

template<typename T>
inline T* GlobalNode::cast() {
  validate_cast_for<T>();
  return reinterpret_cast<T*>(data());
}

template<typename T>
inline const T* GlobalNode::cast() const {
  validate_cast_for<T>();
  return reinterpret_cast<const T*>(data());
}

template<typename T>
void GlobalNode::validate_cast_for() const {
  const auto traits = m_storage_dispatch(StorageMode::k_traits_global, nullptr, nullptr);
  RX_ASSERT(sizeof(T) == ((traits >> 32) & 0xFFFFFFFF_u32), "invalid size");
  RX_ASSERT(alignof(T) == (traits & 0xFFFFFFFF_u32), "invalid allignment");
}

// global_group
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
  return m_global_store.data();
}

template<typename T>
inline constexpr const T* Global<T>::operator&() const {
  return m_global_store.data();
}

template<typename T>
inline constexpr T& Global<T>::operator*() {
  return *m_global_store.data();
}

template<typename T>
constexpr const T& Global<T>::operator*() const {
  return *m_global_store.data();
}

template<typename T>
constexpr T* Global<T>::operator->() {
  return m_global_store.data();
}

template<typename T>
constexpr const T* Global<T>::operator->() const {
  return m_global_store.data();
}

template<typename T>
inline constexpr T* Global<T>::data() {
  return m_global_store.data();
}

template<typename T>
inline constexpr const T* Global<T>::data() const {
  return m_global_store.data();
}

} // namespace rx

#endif // RX_CORE_GLOBAL_H

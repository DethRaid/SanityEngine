#ifndef RX_CORE_INTRUSIVE_XOR_LIST_H
#define RX_CORE_INTRUSIVE_XOR_LIST_H
#include "rx/core/types.h"
#include "rx/core/concepts/no_copy.h"
#include "rx/core/hints/empty_bases.h"
#include "rx/core/utility/exchange.h"

namespace Rx {

// # XOR doubly-linked-list
//
// An intrusive, doubly-linked list where nodes require half the storage since
// the link structure only requires one pointer instead of two.
//
// Like all intrusive containers, you must embed the node in your own structure
// which is called a "link". The use of |push| takes a pointer to this node.
// Retrieving the data associated with the node requires finding the offset
// relative to the node to reconstruct, this link address is given to
// |enumerate_head|, |enumerate_tail| or |node::data| by pointer to the link
// node in your structure.
//
// This should not be used unless you need to reduce the storage costs of your
// nodes. Rex makes use of this for it's globals system since rx::Global<T> is
// quite large and minimizing the size is worthwhile.
//
// 32-bit: 8 bytes
// 64-bit: 16 bytes
struct RX_HINT_EMPTY_BASES intrusive_xor_list
  : Concepts::NoCopy
{
  struct Node;

  constexpr intrusive_xor_list();
  intrusive_xor_list(intrusive_xor_list&& xor_list_);
  intrusive_xor_list& operator=(intrusive_xor_list&& xor_list_);

  void push(Node* _node);

  // 32-bit: 4 bytes
  // 64-bit: 8 bytes
  struct RX_HINT_EMPTY_BASES Node
    : Concepts::NoCopy
  {
    constexpr Node();
    Node(Node&& node_);
    Node& operator=(Node&& node_);

    template<typename T>
    const T* data(Node T::*_link) const;

    template<typename T>
    T* data(Node T::*_link);

  private:
    friend struct intrusive_xor_list;

    Node* m_link;
  };

private:
  Node* m_head;
  Node* m_tail;

  // 32-bit: 12 bytes
  // 64-bit: 24 bytes
  struct RX_HINT_EMPTY_BASES Iterator
    : Concepts::NoCopy
  {
    constexpr Iterator(Node* _node);
    Iterator(Iterator&& iterator_);
    Iterator& operator=(Iterator&& iterator_);

    void next();
    void prev();

  protected:
    Node* m_this;
    Node* m_prev;
    Node* m_next;
  };

public:
  // 32-bit: 12 (base class) + 4 bytes
  // 64-bit: 24 (base class) + 8 bytes
  template<typename T>
  struct Enumerate
    : Iterator
  {
    Enumerate(Enumerate&& enumerate_);
    Enumerate& operator=(Enumerate&& enumerate_);

    operator bool() const;

    T& operator*();
    const T& operator*() const;

    T* operator->();
    const T* operator->() const;

    T* data();
    const T* data() const;

  private:
    friend struct intrusive_xor_list;
    constexpr Enumerate(Node* _root, Node T::*_link);

    Node T::*m_link;
  };

  template<typename T>
  Enumerate<T> enumerate_head(Node T::*_link) const;
  template<typename T>
  Enumerate<T> enumerate_tail(Node T::*_link) const;

  bool is_empty() const;
};

// intrusive_xor_list
inline constexpr intrusive_xor_list::intrusive_xor_list()
  : m_head{nullptr}
  , m_tail{nullptr}
{
}

inline intrusive_xor_list::intrusive_xor_list(intrusive_xor_list&& xor_list_)
  : m_head{Utility::exchange(xor_list_.m_head, nullptr)}
  , m_tail{Utility::exchange(xor_list_.m_tail, nullptr)}
{
}

inline intrusive_xor_list& intrusive_xor_list::operator=(intrusive_xor_list&& xor_list_) {
  m_head = Utility::exchange(xor_list_.m_head, nullptr);
  m_tail = Utility::exchange(xor_list_.m_tail, nullptr);
  return *this;
}

template<typename T>
inline intrusive_xor_list::Enumerate<T> intrusive_xor_list::enumerate_head(Node T::*_link) const {
  return {m_head, _link};
}

template<typename T>
inline intrusive_xor_list::Enumerate<T> intrusive_xor_list::enumerate_tail(Node T::*_link) const {
  return {m_tail, _link};
}

inline bool intrusive_xor_list::is_empty() const {
  return m_head == nullptr;
}

// intrusive_xor_list::node
inline constexpr intrusive_xor_list::Node::Node()
  : m_link{nullptr}
{
}

template<typename T>
inline const T* intrusive_xor_list::Node::data(Node T::*_link) const {
  const auto this_address = reinterpret_cast<UintPtr>(this);
  const auto link_offset = &(reinterpret_cast<const volatile T*>(0)->*_link);
  const auto link_address = reinterpret_cast<UintPtr>(link_offset);
  return reinterpret_cast<const T*>(this_address - link_address);
}

template<typename T>
inline T* intrusive_xor_list::Node::data(Node T::*_link) {
  const auto this_address = reinterpret_cast<UintPtr>(this);
  const auto link_offset = &(reinterpret_cast<const volatile T*>(0)->*_link);
  const auto link_address = reinterpret_cast<UintPtr>(link_offset);
  return reinterpret_cast<T*>(this_address - link_address);
}

// intrusive_xor_list::iterator
inline constexpr intrusive_xor_list::Iterator::Iterator(Node* _node)
  : m_this{_node}
  , m_prev{nullptr}
  , m_next{nullptr}
{
}

inline intrusive_xor_list::Iterator::Iterator(Iterator&& iterator_)
  : m_this{Utility::exchange(iterator_.m_this, nullptr)}
  , m_prev{Utility::exchange(iterator_.m_prev, nullptr)}
  , m_next{Utility::exchange(iterator_.m_next, nullptr)}
{
}

inline intrusive_xor_list::Iterator& intrusive_xor_list::Iterator::operator=(Iterator&& iterator_) {
  m_this = Utility::exchange(iterator_.m_this, nullptr);
  m_prev = Utility::exchange(iterator_.m_prev, nullptr);
  m_next = Utility::exchange(iterator_.m_next, nullptr);
  return *this;
}

// intrusive_xor_list::enumerate
template<typename T>
inline intrusive_xor_list::Enumerate<T>::Enumerate(Enumerate&& enumerate_)
  : Iterator{Utility::move(enumerate_)}
  , m_link{Utility::exchange(enumerate_.m_link, nullptr)}
{
}

template<typename T>
inline intrusive_xor_list::Enumerate<T>& intrusive_xor_list::Enumerate<T>::operator=(Enumerate&& enumerate_) {
  Iterator::operator=(Utility::move(enumerate_));
  m_link = Utility::exchange(enumerate_.m_link, nullptr);
  return *this;
}

template<typename T>
inline constexpr intrusive_xor_list::Enumerate<T>::Enumerate(Node* _root, Node T::*_link)
  : Iterator{_root}
  , m_link{_link}
{
}

template<typename T>
inline intrusive_xor_list::Enumerate<T>::operator bool() const {
  return m_this != nullptr;
}

template<typename T>
inline T& intrusive_xor_list::Enumerate<T>::operator*() {
  return *data();
}

template<typename T>
inline const T& intrusive_xor_list::Enumerate<T>::operator*() const {
  return *data();
}

template<typename T>
inline T* intrusive_xor_list::Enumerate<T>::operator->() {
  return data();
}

template<typename T>
inline const T* intrusive_xor_list::Enumerate<T>::operator->() const {
  return data();
}

template<typename T>
inline T* intrusive_xor_list::Enumerate<T>::data() {
  return m_this->data<T>(m_link);
}

template<typename T>
inline const T* intrusive_xor_list::Enumerate<T>::data() const {
  return m_this->data<T>(m_link);
}

} // namespace rx

#endif // RX_CORE_XOR_LIST_H

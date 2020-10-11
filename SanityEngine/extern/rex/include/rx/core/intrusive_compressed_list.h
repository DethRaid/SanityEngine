#ifndef RX_CORE_INTRUSIVE_COMPRESSED_LIST_H
#define RX_CORE_INTRUSIVE_COMPRESSED_LIST_H

#include "rx/core/types.h"
#include "rx/core/markers.h"

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
struct RX_API IntrusiveCompressedList {
  RX_MARK_NO_COPY(IntrusiveCompressedList);

  struct Node;

  constexpr IntrusiveCompressedList();
  IntrusiveCompressedList(IntrusiveCompressedList&& xor_list_);
  IntrusiveCompressedList& operator=(IntrusiveCompressedList&& xor_list_);

  void push(Node* _node);

  // 32-bit: 4 bytes
  // 64-bit: 8 bytes
  struct Node {
    RX_MARK_NO_COPY(Node);

    constexpr Node();

    template<typename T>
    const T* data(Node T::*_link) const;

    template<typename T>
    T* data(Node T::*_link);

  private:
    friend struct IntrusiveCompressedList;

    Node* m_link;
  };

private:
  Node* m_head;
  Node* m_tail;

  // 32-bit: 12 bytes
  // 64-bit: 24 bytes
  struct RX_API Iterator {
    RX_MARK_NO_COPY(Iterator);

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
    friend struct IntrusiveCompressedList;

    constexpr Enumerate(Node* _root, Node T::*_link);

    Node T::*m_link;
  };

  template<typename T>
  Enumerate<T> enumerate_head(Node T::*_link) const;
  template<typename T>
  Enumerate<T> enumerate_tail(Node T::*_link) const;

  bool is_empty() const;
};

// IntrusiveCompressedList
inline constexpr IntrusiveCompressedList::IntrusiveCompressedList()
  : m_head{nullptr}
  , m_tail{nullptr}
{
}

inline IntrusiveCompressedList::IntrusiveCompressedList(IntrusiveCompressedList&& xor_list_)
  : m_head{Utility::exchange(xor_list_.m_head, nullptr)}
  , m_tail{Utility::exchange(xor_list_.m_tail, nullptr)}
{
}

inline IntrusiveCompressedList& IntrusiveCompressedList::operator=(IntrusiveCompressedList&& xor_list_) {
  m_head = Utility::exchange(xor_list_.m_head, nullptr);
  m_tail = Utility::exchange(xor_list_.m_tail, nullptr);
  return *this;
}

template<typename T>
inline IntrusiveCompressedList::Enumerate<T> IntrusiveCompressedList::enumerate_head(Node T::*_link) const {
  return {m_head, _link};
}

template<typename T>
inline IntrusiveCompressedList::Enumerate<T> IntrusiveCompressedList::enumerate_tail(Node T::*_link) const {
  return {m_tail, _link};
}

inline bool IntrusiveCompressedList::is_empty() const {
  return m_head == nullptr;
}

// IntrusiveCompressedList::Node
inline constexpr IntrusiveCompressedList::Node::Node()
  : m_link{nullptr}
{
}

template<typename T>
inline const T* IntrusiveCompressedList::Node::data(Node T::*_link) const {
  const auto this_address = reinterpret_cast<UintPtr>(this);
  const auto link_offset = &(reinterpret_cast<const volatile T*>(0)->*_link);
  const auto link_address = reinterpret_cast<UintPtr>(link_offset);
  return reinterpret_cast<const T*>(this_address - link_address);
}

template<typename T>
inline T* IntrusiveCompressedList::Node::data(Node T::*_link) {
  const auto this_address = reinterpret_cast<UintPtr>(this);
  const auto link_offset = &(reinterpret_cast<const volatile T*>(0)->*_link);
  const auto link_address = reinterpret_cast<UintPtr>(link_offset);
  return reinterpret_cast<T*>(this_address - link_address);
}

// IntrusiveCompressedList::Iterator
inline constexpr IntrusiveCompressedList::Iterator::Iterator(Node* _node)
  : m_this{_node}
  , m_prev{nullptr}
  , m_next{nullptr}
{
}

inline IntrusiveCompressedList::Iterator::Iterator(Iterator&& iterator_)
  : m_this{Utility::exchange(iterator_.m_this, nullptr)}
  , m_prev{Utility::exchange(iterator_.m_prev, nullptr)}
  , m_next{Utility::exchange(iterator_.m_next, nullptr)}
{
}

inline IntrusiveCompressedList::Iterator& IntrusiveCompressedList::Iterator::operator=(Iterator&& iterator_) {
  m_this = Utility::exchange(iterator_.m_this, nullptr);
  m_prev = Utility::exchange(iterator_.m_prev, nullptr);
  m_next = Utility::exchange(iterator_.m_next, nullptr);
  return *this;
}

// intrusive_xor_list::enumerate
template<typename T>
inline IntrusiveCompressedList::Enumerate<T>::Enumerate(Enumerate&& enumerate_)
  : Iterator{Utility::move(enumerate_)}
  , m_link{Utility::exchange(enumerate_.m_link, nullptr)}
{
}

template<typename T>
inline IntrusiveCompressedList::Enumerate<T>& IntrusiveCompressedList::Enumerate<T>::operator=(Enumerate&& enumerate_) {
  Iterator::operator=(Utility::move(enumerate_));
  m_link = Utility::exchange(enumerate_.m_link, nullptr);
  return *this;
}

template<typename T>
inline constexpr IntrusiveCompressedList::Enumerate<T>::Enumerate(Node* _root, Node T::*_link)
  : Iterator{_root}
  , m_link{_link}
{
}

template<typename T>
inline IntrusiveCompressedList::Enumerate<T>::operator bool() const {
  return m_this != nullptr;
}

template<typename T>
inline T& IntrusiveCompressedList::Enumerate<T>::operator*() {
  return *data();
}

template<typename T>
inline const T& IntrusiveCompressedList::Enumerate<T>::operator*() const {
  return *data();
}

template<typename T>
inline T* IntrusiveCompressedList::Enumerate<T>::operator->() {
  return data();
}

template<typename T>
inline const T* IntrusiveCompressedList::Enumerate<T>::operator->() const {
  return data();
}

template<typename T>
inline T* IntrusiveCompressedList::Enumerate<T>::data() {
  return m_this->data<T>(m_link);
}

template<typename T>
inline const T* IntrusiveCompressedList::Enumerate<T>::data() const {
  return m_this->data<T>(m_link);
}

} // namespace Rx

#endif // RX_CORE_INTRUSIVE_COMPRESSED_LIST

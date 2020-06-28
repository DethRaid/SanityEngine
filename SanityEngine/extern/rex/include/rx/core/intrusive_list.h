#ifndef RX_CORE_INTRUSIVE_LIST_H
#define RX_CORE_INTRUSIVE_LIST_H
#include "rx/core/types.h"
#include "rx/core/markers.h"

#include "rx/core/utility/exchange.h"

namespace Rx {

// # Doubly-linked-list
//
// An intrusive, doubly-linked list.
//
// Like all intrusive containers, you must embed the node in your own structure
// which is called a "link". The use of |push_front| and |push_back| takes a
// pointer to this node. Retrieving the data associated with the node requires
// finding the offset relative to the node to reconstruct, this link address is
// given to |enumerate_head|, |enumerate_tail| or |node::data| by pointer to
// the link node in your structure.
//
// 32-bit: 8 bytes
// 64-bit: 16 bytes
struct IntrusiveList {
  RX_MARK_NO_COPY(IntrusiveList);

  struct Node;

  constexpr IntrusiveList();
  IntrusiveList(IntrusiveList&& _list);

  IntrusiveList& operator=(IntrusiveList&& list_);

  void push_front(Node* _node);
  void push_back(Node* _node);

  void erase(Node* node_);

  Node* pop_front();
  Node* pop_back();

  // 32-bit: 8 bytes
  // 64-bit: 16 bytes
  struct Node {
    RX_MARK_NO_COPY(Node);

    constexpr Node();
    Node(Node&& node_);
    Node& operator=(Node&& node_);

    template<typename T>
    const T* data(Node T::*_link) const;

    template<typename T>
    T* data(Node T::*_link);

  private:
    friend struct IntrusiveList;

    Node* m_next;
    Node* m_prev;
  };

  // 32-bit: 8 bytes
  // 64-bit: 16 bytes
  template<typename T>
  struct Enumerate {
    RX_MARK_NO_COPY(Enumerate);

    constexpr Enumerate(Node* _root, Node T::*_link);
    Enumerate(Enumerate&& enumerate_);
    Enumerate& operator=(Enumerate&& enumerate_);

    operator bool() const;

    T& operator*();
    const T& operator*() const;

    T* operator->();
    const T* operator->() const;

    T* data();
    const T* data() const;

    void next();
    void prev();

  private:
    Node* m_this;
    Node T::*m_link;
  };

  template<typename T>
  Enumerate<T> enumerate_head(Node T::*_link) const;
  template<typename T>
  Enumerate<T> enumerate_tail(Node T::*_link) const;

  bool is_empty() const;

private:
  Node* m_head;
  Node* m_tail;
};

// IntrusiveList
inline constexpr IntrusiveList::IntrusiveList()
  : m_head{nullptr}
  , m_tail{nullptr}
{
}

inline IntrusiveList::IntrusiveList(IntrusiveList&& list_)
  : m_head{Utility::exchange(list_.m_head, nullptr)}
  , m_tail{Utility::exchange(list_.m_tail, nullptr)}
{
}

inline IntrusiveList& IntrusiveList::operator=(IntrusiveList&& list_) {
  m_head = Utility::exchange(list_.m_head, nullptr);
  m_tail = Utility::exchange(list_.m_tail, nullptr);
  return *this;
}

template<typename T>
inline IntrusiveList::Enumerate<T> IntrusiveList::enumerate_head(Node T::*_link) const {
  return {m_head, _link};
}

template<typename T>
inline IntrusiveList::Enumerate<T> IntrusiveList::enumerate_tail(Node T::*_link) const {
  return {m_tail, _link};
}

inline bool IntrusiveList::is_empty() const {
  return m_head == nullptr;
}

// IntrusiveList::node
inline constexpr IntrusiveList::Node::Node()
  : m_next{nullptr}
  , m_prev{nullptr}
{
}

inline IntrusiveList::Node::Node(Node&& node_)
  : m_next{Utility::exchange(node_.m_next, nullptr)}
  , m_prev{Utility::exchange(node_.m_prev, nullptr)}
{
}

inline IntrusiveList::Node& IntrusiveList::Node::operator=(Node&& node_) {
  m_next = Utility::exchange(node_.m_next, nullptr);
  m_prev = Utility::exchange(node_.m_prev, nullptr);
  return *this;
}

template<typename T>
inline const T* IntrusiveList::Node::data(Node T::*_link) const {
  const auto this_address = reinterpret_cast<UintPtr>(this);
  const auto link_offset = &(reinterpret_cast<const volatile T*>(0)->*_link);
  const auto link_address = reinterpret_cast<UintPtr>(link_offset);
  return reinterpret_cast<const T*>(this_address - link_address);
}

template<typename T>
inline T* IntrusiveList::Node::data(Node T::*_link) {
  const auto this_address = reinterpret_cast<UintPtr>(this);
  const auto link_offset = &(reinterpret_cast<const volatile T*>(0)->*_link);
  const auto link_address = reinterpret_cast<UintPtr>(link_offset);
  return reinterpret_cast<T*>(this_address - link_address);
}

// IntrusiveList::enumerate
template<typename T>
inline constexpr IntrusiveList::Enumerate<T>::Enumerate(Node* _root, Node T::*_link)
  : m_this{_root}
  , m_link{_link}
{
}

template<typename T>
inline IntrusiveList::Enumerate<T>::Enumerate(Enumerate&& enumerate_)
  : m_this{Utility::exchange(enumerate_.m_this, nullptr)}
  , m_link{Utility::exchange(enumerate_.m_link, nullptr)}
{
}

template<typename T>
inline IntrusiveList::Enumerate<T>& IntrusiveList::Enumerate<T>::operator=(Enumerate&& enumerate_) {
  m_this = Utility::exchange(enumerate_.m_this, nullptr);
  m_link = Utility::exchange(enumerate_.m_link, nullptr);
  return *this;
}

template<typename T>
inline IntrusiveList::Enumerate<T>::operator bool() const {
  return m_this;
}

template<typename T>
inline void IntrusiveList::Enumerate<T>::next() {
  m_this = m_this->m_next;
}

template<typename T>
inline void IntrusiveList::Enumerate<T>::prev() {
  m_this = m_this->m_prev;
}

template<typename T>
inline T& IntrusiveList::Enumerate<T>::operator*() {
  return *data();
}

template<typename T>
inline const T& IntrusiveList::Enumerate<T>::operator*() const {
  return *data();
}

template<typename T>
inline T* IntrusiveList::Enumerate<T>::operator->() {
  return data();
}

template<typename T>
inline const T* IntrusiveList::Enumerate<T>::operator->() const {
  return data();
}

template<typename T>
inline T* IntrusiveList::Enumerate<T>::data() {
  return m_this->data<T>(m_link);
}

template<typename T>
inline const T* IntrusiveList::Enumerate<T>::data() const {
  return m_this->data<T>(m_link);
}

} // namespace rx

#endif // RX_CORE_INTRUSIVE_LIST_H

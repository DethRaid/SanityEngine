#include "rx/core/intrusive_list.h"

namespace Rx {

void IntrusiveList::push_front(Node* node_) {
  if (m_head) {
    m_head->m_prev = node_;
    node_->m_prev = nullptr;
    node_->m_next = m_head;
    m_head = node_;
  } else {
    m_head = node_;
    m_tail = node_;
    node_->m_next = nullptr;
    node_->m_prev = nullptr;
  }
}

void IntrusiveList::push_back(Node* node_) {
  if (m_tail) {
    m_tail->m_next = node_;
    node_->m_prev = m_tail;
    node_->m_next = nullptr;
    m_tail = node_;
  } else {
    m_head = node_;
    m_tail = node_;
    node_->m_next = nullptr;
    node_->m_prev = nullptr;
  }
}

void IntrusiveList::erase(Node* node_) {
  if (node_) {
    if (node_->m_next) {
      node_->m_next->m_prev = node_->m_prev;
    }
    if (node_->m_prev) {
      node_->m_prev->m_next = node_->m_next;
    }
    if (m_head == node_) {
      m_head = node_->m_next;
    }
    if (m_tail == node_) {
      m_tail = node_->m_prev;
    }
  }
}

IntrusiveList::Node* IntrusiveList::pop_front() {
  if (Node* link = m_head) {
    if (link->m_next) {
      link->m_next->m_prev = link->m_prev;
    }
    if (link->m_prev) {
      link->m_prev->m_next = link->m_next;
    }
    if (link == m_head) {
      m_head = link->m_next;
    }
    if (link == m_tail) {
      m_tail = link->m_prev;
    }
    return link;
  }
  return nullptr;
}

IntrusiveList::Node* IntrusiveList::pop_back() {
  if (Node* link = m_tail) {
    if (link->m_next) {
      link->m_next->m_prev = link->m_prev;
    }
    if (link->m_prev) {
      link->m_prev->m_next = link->m_next;
    }
    if (link == m_head) {
      m_head = link->m_next;
    }
    if (link == m_tail) {
      m_tail = link->m_prev;
    }
    return link;
  }
  return nullptr;
}

} // namespace rx

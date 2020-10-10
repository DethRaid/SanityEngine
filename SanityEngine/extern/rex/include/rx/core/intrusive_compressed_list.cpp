#include "rx/core/intrusive_compressed_list.h"
#include "rx/core/types.h"

namespace Rx {

static IntrusiveCompressedList::Node* xor_nodes(IntrusiveCompressedList::Node* _x, IntrusiveCompressedList::Node* _y) {
  const auto x = reinterpret_cast<UintPtr>(_x);
  const auto y = reinterpret_cast<UintPtr>(_y);
  return reinterpret_cast<IntrusiveCompressedList::Node*>(x ^ y);
}

void IntrusiveCompressedList::push(Node* node_) {
  if (!m_head) {
    m_head = node_;
    m_tail = node_;
  } else {
    node_->m_link = xor_nodes(m_tail, nullptr);
    m_tail->m_link = xor_nodes(node_, xor_nodes(m_tail->m_link, nullptr));
    m_tail = node_;
  }
}

void IntrusiveCompressedList::Iterator::next() {
  if (m_this) {
    m_next = xor_nodes(m_prev, m_this->m_link);
    m_prev = Utility::exchange(m_this, m_next);
  }
}

void IntrusiveCompressedList::Iterator::prev() {
  next();
}

} // namespace rx

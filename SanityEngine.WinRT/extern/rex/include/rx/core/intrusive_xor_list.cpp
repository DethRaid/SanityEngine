#include "rx/core/intrusive_xor_list.h"
#include "rx/core/types.h"

namespace Rx {

static intrusive_xor_list::Node* xor_nodes(intrusive_xor_list::Node* _x, intrusive_xor_list::Node* _y) {
  const auto x = reinterpret_cast<UintPtr>(_x);
  const auto y = reinterpret_cast<UintPtr>(_y);
  return reinterpret_cast<intrusive_xor_list::Node*>(x ^ y);
}

void intrusive_xor_list::push(Node* node_) {
  if (!m_head) {
    m_head = node_;
    m_tail = node_;
  } else {
    node_->m_link = xor_nodes(m_tail, nullptr);
    m_tail->m_link = xor_nodes(node_, xor_nodes(m_tail->m_link, nullptr));
    m_tail = node_;
  }
}

void intrusive_xor_list::Iterator::next() {
  if (m_this) {
    m_next = xor_nodes(m_prev, m_this->m_link);
    m_prev = Utility::exchange(m_this, m_next);
  }
}

void intrusive_xor_list::Iterator::prev() {
  next();
}

} // namespace rx

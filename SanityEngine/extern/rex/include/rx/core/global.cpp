#include <string.h> // strcmp
#include <stdlib.h> // malloc, free

#include "rx/core/global.h"
#include "rx/core/log.h"

#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/concurrency/spin_lock.h"

namespace Rx {

RX_LOG("global", logger);

static Concurrency::SpinLock g_lock;

static GlobalGroup g_group_system{"system"};

// Early initialization of globals depends on heap allocation for parameters to
// globals. Since HeapAllocator is a global itself and implements the necessary
// functionality for ensuring memory is always aligned by Allocator::ALIGNMENT
// we need to implement a boot strapping allocator that ensures this alignment.
Byte* GlobalNode::allocate(Size _size) {
  auto offset = Memory::Allocator::ALIGNMENT - 1 + sizeof(Byte*);
  if (auto p1 = reinterpret_cast<Byte*>(malloc(_size + offset))) {
    auto p2 = reinterpret_cast<Byte**>((reinterpret_cast<UintPtr>(p1) + offset) & ~(Memory::Allocator::ALIGNMENT- 1));
    p2[-1] = p1;
    return reinterpret_cast<Byte*>(p2);
  }
  return nullptr;
}

void GlobalNode::deallocate(Byte* _data) {
  free(reinterpret_cast<void**>(_data)[-1]);
}

void GlobalNode::init() {
  const auto flags = m_argument_store.as_tag();
  if (flags & INITIALIZED) {
    return;
  }

  m_storage_dispatch(StorageMode::INIT, data(), m_argument_store.as_ptr());
  m_argument_store.retag(flags | INITIALIZED);

  {
    Concurrency::ScopeLock lock{g_lock};
    Globals::s_initialized_list.push_back(&m_initialized);
  }
}

void GlobalNode::fini() {
  const auto flags = m_argument_store.as_tag();
  if (!(flags & INITIALIZED)) {
    return;
  }

  if (flags & ARGUMENTS) {
    m_storage_dispatch(StorageMode::FINI, data(), m_argument_store.as_ptr());
    deallocate(m_argument_store.as_ptr());
    m_argument_store = nullptr;
  } else {
    m_storage_dispatch(StorageMode::FINI, data(), nullptr);
  }

  {
    Concurrency::ScopeLock lock{g_lock};
    Globals::s_initialized_list.erase(&m_initialized);
  }
}

// GlobalGroup
GlobalNode* GlobalGroup::find(const char* _name) {
  for (auto node = m_list.enumerate_head(&GlobalNode::m_grouped); node; node.next()) {
    if (!strcmp(node->name(), _name)) {
      return node.data();
    }
  }
  return nullptr;
}

void GlobalGroup::init() {
  for (auto node = m_list.enumerate_head(&GlobalNode::m_grouped); node; node.next()) {
    node->init();
  }
}

void GlobalGroup::fini() {
  for (auto node = m_list.enumerate_tail(&GlobalNode::m_grouped); node; node.prev()) {
    node->fini();
  }
}

// Globals
GlobalGroup* Globals::find(const char* _name) {
  for (auto group = s_group_list.enumerate_head(&GlobalGroup::m_link); group; group.next()) {
    if (!strcmp(group->name(), _name)) {
      return group.data();
    }
  }
  return nullptr;
}

bool Globals::link() {
  // Link ungrouped globals from |s_node_list| managed by |GlobalNode::m_ungrouped|
  // with the appropriate group given by |GlobalGroup::m_list|, which is managed
  // by |GlobalNode::m_grouped| when the given global shares the same group
  // name as the group.
  Concurrency::ScopeLock lock{g_lock};
  for (auto node = s_node_list.enumerate_head(&GlobalNode::m_ungrouped); node; node.next()) {
    bool unlinked = true;
    for (auto group = s_group_list.enumerate_head(&GlobalGroup::m_link); group; group.next()) {
      if (!strcmp(node->m_group, group->name())) {
        group->m_list.push(&node->m_grouped);
        unlinked = false;
      }
    }

    if (unlinked) {
      return false;
    }
  }

  return true;
}

void Globals::init() {
  for (auto group = s_group_list.enumerate_head(&GlobalGroup::m_link); group; group.next()) {
    group->init();
  }
}

void Globals::fini() {
  // Finalize all globals in the reverse order they were initialized.
  for (auto node = s_initialized_list.enumerate_tail(&GlobalNode::m_initialized); node; node.prev()) {
    node->fini();
  }
}

void Globals::link(GlobalNode* _node) {
  Concurrency::ScopeLock lock{g_lock};
  s_node_list.push(&_node->m_ungrouped);
}

void Globals::link(GlobalGroup* _group) {
  Concurrency::ScopeLock lock{g_lock};
  s_group_list.push(&_group->m_link);
}

} // namespace rx

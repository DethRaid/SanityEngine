#include <string.h> // strcmp
#include <stdlib.h> // malloc, free

#include "rx/core/global.h"
#include "rx/core/log.h"

#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/concurrency/spin_lock.h"

namespace Rx {

RX_LOG("global", logger);

static Concurrency::SpinLock g_lock;

// GlobalNode
void GlobalNode::init_global() {
  const auto flags = m_argument_store.as_tag();
  if (!(flags & k_enabled)) {
    return;
  }

  RX_ASSERT(!(flags & k_initialized), "already initialized");
  logger->verbose("%p init: %s/%s", this, m_group, m_name);

  m_storage_dispatch(StorageMode::k_init_global, data(), m_argument_store.as_ptr());

  m_argument_store.retag(flags | k_initialized);
}

void GlobalNode::fini_global() {
  const auto flags = m_argument_store.as_tag();

  if (!(flags & k_enabled)) {
    return;
  }

  RX_ASSERT(flags & k_initialized, "not initialized");
  logger->verbose("%p fini: %s/%s", this, m_group, m_name);

  m_storage_dispatch(StorageMode::k_fini_global, data(), nullptr);
  if (flags & k_arguments) {
    auto argument_store = m_argument_store.as_ptr();
    m_storage_dispatch(StorageMode::k_fini_arguments, nullptr, argument_store);
    reallocate_arguments(argument_store, 0);
  }

  m_argument_store.retag(flags & ~k_initialized);
}

void GlobalNode::init() {
  const auto flags = m_argument_store.as_tag();
  RX_ASSERT(!(flags & k_initialized), "already initialized");

  m_storage_dispatch(StorageMode::k_init_global, data(), m_argument_store.as_ptr());

  m_argument_store.retag((flags & ~k_enabled) | k_initialized);
}

void GlobalNode::fini() {
  const auto flags = m_argument_store.as_tag();
  RX_ASSERT(flags & k_initialized, "not initialized");

  m_storage_dispatch(StorageMode::k_fini_global, data(), nullptr);
  if (flags & k_arguments) {
    auto argument_store = m_argument_store.as_ptr();
    m_storage_dispatch(StorageMode::k_fini_arguments, nullptr, argument_store);
    reallocate_arguments(argument_store, 0);
  }

  m_argument_store.retag((flags & ~k_enabled) | k_initialized);
}

Byte* GlobalNode::reallocate_arguments(Byte* _existing, Size _size) {
  if (_existing && _size == 0) {
    free(_existing);
    return nullptr;
  }
  return reinterpret_cast<Byte*>(realloc(nullptr, _size));
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

void GlobalGroup::init_global() {
  for (auto node = m_list.enumerate_head(&GlobalNode::m_grouped); node; node.next()) {
    node->init_global();
  }
}

void GlobalGroup::fini_global() {
  for (auto node = m_list.enumerate_tail(&GlobalNode::m_grouped); node; node.prev()) {
    node->fini_global();
  }
}

// globals
GlobalGroup* Globals::find(const char* _name) {
  for (auto group = s_group_list.enumerate_head(&GlobalGroup::m_link); group; group.next()) {
    if (!strcmp(group->name(), _name)) {
      return group.data();
    }
  }
  return nullptr;
}

void Globals::link() {
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
        break;
      }
    }

    if (unlinked) {
      // NOTE(dweiler): If you've hit this code-enforced crash it means there
      // exists an rx::Global<T> that is associated with a group by name which
      // doesn't exist. This can be caused by misnaming the group in the
      // global's constructor, or because the rx::GlobalGroup with that name
      // doesn't exist in any translation unit.
      *reinterpret_cast<volatile int*>(0) = 0;
    }
  }
}

void Globals::init() {
  for (auto group = s_group_list.enumerate_head(&GlobalGroup::m_link); group; group.next()) {
    group->init_global();
  }
}

void Globals::fini() {
  for (auto group = s_group_list.enumerate_tail(&GlobalGroup::m_link); group; group.prev()) {
    group->fini_global();
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

static GlobalGroup g_group_system{"system"};

} // namespace rx

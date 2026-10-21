// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_LINKED_BINDING_H_
#define ELECTRON_SHELL_COMMON_NODE_LINKED_BINDING_H_

// Lightweight alternative to node_includes.h for files that only need to
// register a linked binding. Pulls in the public node.h only.

#ifdef NODE_SHARED_MODE
#define BUILDING_NODE_EXTENSION
#endif

#undef debug_string    // This is defined in macOS SDK in AssertMacros.h.
#undef require_string  // This is defined in macOS SDK in AssertMacros.h.

#include "electron/push_and_undef_node_defines.h"

#include "node.h"

#include "electron/pop_node_defines.h"

// Alternative to NODE_BINDING_CONTEXT_AWARE_X.
// Allows to explicitly register builtin bindings instead of using
// __attribute__((constructor)), and to look one up without a node::Environment
// (NodeBindings::GetLinkedBinding).
// Same expansion as node's NODE_BINDING_CONTEXT_AWARE_CPP(modname, regfunc,
// nullptr, NM_F_LINKED), spelled out with the public node.h names
// (NM_F_LINKED == node::ModuleFlags::kLinked; node static_asserts this).
#define NODE_LINKED_BINDING_CONTEXT_AWARE(modname, regfunc) \
  static node::node_module _module = {                      \
      NODE_MODULE_VERSION,                                  \
      node::ModuleFlags::kLinked,                           \
      nullptr,                                              \
      __FILE__,                                             \
      nullptr,                                              \
      (node::addon_context_register_func)(regfunc),         \
      NODE_STRINGIFY(modname),                              \
      nullptr,                                              \
      nullptr};                                             \
  void _register_##modname() {                              \
    node_module_register(&_module);                         \
  }                                                         \
  node::node_module* get_linked_module_##modname() {        \
    return &_module;                                        \
  }

#endif  // ELECTRON_SHELL_COMMON_NODE_LINKED_BINDING_H_

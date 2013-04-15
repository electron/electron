// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <stdio.h>

#include "vendor/node/src/node.h"
#include "vendor/node/src/node_version.h"

namespace atom {

namespace api {

#undef NODE_EXT_LIST_START
#undef NODE_EXT_LIST_ITEM
#undef NODE_EXT_LIST_END

#define NODE_EXT_LIST_START
#define NODE_EXT_LIST_ITEM NODE_MODULE_DECL
#define NODE_EXT_LIST_END

#include "browser/api/atom_api_extensions.h"

#undef NODE_EXT_LIST_START
#undef NODE_EXT_LIST_ITEM
#undef NODE_EXT_LIST_END

#define NODE_EXT_STRING(x) &x ## _module,
#define NODE_EXT_LIST_START node::node_module_struct *node_module_list[] = {
#define NODE_EXT_LIST_ITEM NODE_EXT_STRING
#define NODE_EXT_LIST_END NULL};

#include "browser/api/atom_api_extensions.h" // NOLINT

node::node_module_struct* get_builtin_module(const char *name) {
  char buf[128];
  node::node_module_struct *cur = NULL;
  snprintf(buf, sizeof(buf), "atom_api_%s", name);
  /* TODO: you could look these up in a hash, but there are only
   * a few, and once loaded they are cached. */
  for (int i = 0; node_module_list[i] != NULL; i++) {
    cur = node_module_list[i];
    if (strcmp(cur->modname, buf) == 0) {
      return cur;
    }
  }

  return NULL;
}

}  // namespace api

}  // namespace atom

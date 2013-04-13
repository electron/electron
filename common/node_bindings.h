// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RAVE_COMMON_NODE_BINDINGS_
#define RAVE_COMMON_NODE_BINDINGS_

#include "base/basictypes.h"

namespace atom {

class NodeBindings {
 public:
  NodeBindings();
  virtual ~NodeBindings();

  // Setup everything including V8, libuv and node.js main script.
  void Initialize();

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeBindings);
};

}  // namespace at

#endif  // RAVE_COMMON_NODE_BINDINGS_

// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_BINDINGS_LINUX_H_
#define ATOM_COMMON_NODE_BINDINGS_LINUX_H_

#include "base/compiler_specific.h"
#include "common/node_bindings.h"

namespace atom {

class NodeBindingsLinux : public NodeBindings {
 public:
  explicit NodeBindingsLinux(bool is_browser);
  virtual ~NodeBindingsLinux();

 private:
  virtual void PollEvents() OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsLinux);
};

}  // namespace atom

#endif  // ATOM_COMMON_NODE_BINDINGS_LINUX_H_

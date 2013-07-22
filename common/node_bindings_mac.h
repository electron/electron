// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_BINDINGS_MAC_
#define ATOM_COMMON_NODE_BINDINGS_MAC_

#include "base/compiler_specific.h"
#include "common/node_bindings.h"

namespace atom {

class NodeBindingsMac : public NodeBindings {
 public:
  explicit NodeBindingsMac(bool is_browser);
  virtual ~NodeBindingsMac();

 private:
  virtual void PollEvents() OVERRIDE;

  // Kqueue to poll for uv's backend fd.
  int kqueue_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsMac);
};

}  // namespace atom

#endif  // ATOM_COMMON_NODE_BINDINGS_MAC_

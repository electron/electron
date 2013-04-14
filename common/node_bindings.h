// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RAVE_COMMON_NODE_BINDINGS_
#define RAVE_COMMON_NODE_BINDINGS_

#include "base/basictypes.h"

namespace atom {

class NodeBindings {
 public:
  static NodeBindings* Create(bool is_browser);

  virtual ~NodeBindings();

  // Setup V8, libuv and the process object.
  virtual void Initialize();

  // Load node.js main script.
  virtual void Load();

  // Prepare for message loop integration.
  virtual void PrepareMessageLoop() = 0;

  // Do message loop integration.
  virtual void RunMessageLoop() = 0;

 protected:
  explicit NodeBindings(bool is_browser);

  bool is_browser_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeBindings);
};

}  // namespace atom

#endif  // RAVE_COMMON_NODE_BINDINGS_

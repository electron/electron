// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NODE_DEBUGGER_H_
#define ATOM_BROWSER_NODE_DEBUGGER_H_

#include "base/macros.h"

namespace node {
class Environment;
class MultiIsolatePlatform;
}  // namespace node

namespace atom {

// Add support for node's "--inspect" switch.
class NodeDebugger {
 public:
  explicit NodeDebugger(node::Environment* env);
  ~NodeDebugger();

  void Start(node::MultiIsolatePlatform* platform);

 private:
  node::Environment* env_;

  DISALLOW_COPY_AND_ASSIGN(NodeDebugger);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NODE_DEBUGGER_H_

// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NODE_DEBUGGER_H_
#define SHELL_BROWSER_NODE_DEBUGGER_H_

#include "base/macros.h"

namespace node {
class Environment;
}  // namespace node

namespace v8_inspector {
class V8InspectorClient;
}  // namespace v8_inspector

namespace electron {

// Add support for node's "--inspect" switch.
class NodeDebugger {
 public:
  explicit NodeDebugger(node::Environment* env);
  NodeDebugger(node::Environment* env,
               v8_inspector::V8InspectorClient* inspector);
  ~NodeDebugger();

  void Start();
  void Stop();

 private:
  node::Environment* env_;
  v8_inspector::V8InspectorClient* client_;

  DISALLOW_COPY_AND_ASSIGN(NodeDebugger);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NODE_DEBUGGER_H_

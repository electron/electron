// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NODE_DEBUGGER_H_
#define ATOM_BROWSER_NODE_DEBUGGER_H_

#include "base/basictypes.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace atom {

// Add support for node's "--debug" switch.
class NodeDebugger {
 public:
  NodeDebugger();
  virtual ~NodeDebugger();

 private:
  static void DispatchDebugMessagesInMainThread(uv_async_t* handle);
  static void DispatchDebugMessagesInMsgThread();

  static uv_async_t dispatch_debug_messages_async_;

  DISALLOW_COPY_AND_ASSIGN(NodeDebugger);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NODE_DEBUGGER_H_

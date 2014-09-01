// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/node_debugger.h"

#include <string>

#include "atom/common/atom_version.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "v8/include/v8.h"
#include "v8/include/v8-debug.h"

#include "atom/common/node_includes.h"

namespace atom {

// static
uv_async_t NodeDebugger::dispatch_debug_messages_async_;

NodeDebugger::NodeDebugger() {
  uv_async_init(uv_default_loop(),
                &dispatch_debug_messages_async_,
                DispatchDebugMessagesInMainThread);
  uv_unref(reinterpret_cast<uv_handle_t*>(&dispatch_debug_messages_async_));

  bool use_debug_agent = false;
  int port = 5858;
  bool wait_for_connection = false;

  std::string port_str;
  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch("debug")) {
    use_debug_agent = true;
    port_str = cmd->GetSwitchValueASCII("debug");
  }
  if (cmd->HasSwitch("debug-brk")) {
    use_debug_agent = true;
    wait_for_connection = true;
    port_str = cmd->GetSwitchValueASCII("debug-brk");
  }

  if (use_debug_agent) {
    if (!port_str.empty())
      base::StringToInt(port_str, &port);
#if 0
    v8::Debug::EnableAgent("atom-shell " ATOM_VERSION, port,
                           wait_for_connection);
    v8::Debug::SetDebugMessageDispatchHandler(DispatchDebugMessagesInMsgThread,
                                              false);
#endif
  }
}

NodeDebugger::~NodeDebugger() {
}

// static
void NodeDebugger::DispatchDebugMessagesInMainThread(uv_async_t* handle) {
  v8::Debug::ProcessDebugMessages();
}

// static
void NodeDebugger::DispatchDebugMessagesInMsgThread() {
  uv_async_send(&dispatch_debug_messages_async_);
}

}  // namespace atom

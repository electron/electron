// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/node_debugger.h"

#include <string>

#include "atom/common/atom_version.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "v8/include/v8-debug.h"

namespace atom {

NodeDebugger::NodeDebugger()
    : thread_("NodeDebugger"),
      weak_factory_(this) {
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

    if (!thread_.Start()) {
      LOG(ERROR) << "Unable to start debugger thread";
      return;
    }

    net::IPAddressNumber ip_number;
    if (!net::ParseIPLiteralToNumber("127.0.0.1", &ip_number)) {
      LOG(ERROR) << "Unable to convert ip address";
      return;
    }

    server_socket_.reset(new net::TCPServerSocket(NULL, net::NetLog::Source()));
    server_socket_->Listen(net::IPEndPoint(ip_number, port), 1);

    thread_.message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&NodeDebugger::DoAcceptLoop, weak_factory_.GetWeakPtr()));
  }
}

NodeDebugger::~NodeDebugger() {
  thread_.Stop();
}

void NodeDebugger::DoAcceptLoop() {
}

}  // namespace atom

// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/node_debugger.h"

#include <string>

#include "atom/common/atom_version.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "net/socket/tcp_listen_socket.h"
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

    // Start a new IO thread.
    base::Thread::Options options;
    options.message_loop_type = base::MessageLoop::TYPE_IO;
    if (!thread_.StartWithOptions(options)) {
      LOG(ERROR) << "Unable to start debugger thread";
      return;
    }

    // Start the server in new IO thread.
    thread_.message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&NodeDebugger::StartServer, weak_factory_.GetWeakPtr(),
                   port));
  }
}

NodeDebugger::~NodeDebugger() {
  thread_.Stop();
}

void NodeDebugger::StartServer(int port) {
  server_ = net::TCPListenSocket::CreateAndListen("127.0.0.1", port, this);
  if (!server_) {
    LOG(ERROR) << "Cannot start debugger server";
    return;
  }
}

void NodeDebugger::DidAccept(net::StreamListenSocket* server,
                             scoped_ptr<net::StreamListenSocket> socket) {
  // Only accept one session.
  if (accepted_socket_)
    return;

  accepted_socket_ = socket.Pass();
}

void NodeDebugger::DidRead(net::StreamListenSocket* socket,
                           const char* data,
                           int len) {
}

void NodeDebugger::DidClose(net::StreamListenSocket* socket) {
  accepted_socket_.reset();
}

}  // namespace atom

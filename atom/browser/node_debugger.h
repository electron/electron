// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NODE_DEBUGGER_H_
#define ATOM_BROWSER_NODE_DEBUGGER_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "net/socket/stream_listen_socket.h"

namespace atom {

// Add support for node's "--debug" switch.
class NodeDebugger : public net::StreamListenSocket::Delegate {
 public:
  NodeDebugger();
  virtual ~NodeDebugger();

 private:
  void StartServer(int port);

  // net::StreamListenSocket::Delegate:
  virtual void DidAccept(net::StreamListenSocket* server,
                         scoped_ptr<net::StreamListenSocket> socket) OVERRIDE;
  virtual void DidRead(net::StreamListenSocket* socket,
                       const char* data,
                       int len) OVERRIDE;
  virtual void DidClose(net::StreamListenSocket* socket) OVERRIDE;

  base::Thread thread_;
  scoped_ptr<net::StreamListenSocket> server_;
  scoped_ptr<net::StreamListenSocket> accepted_socket_;

  std::string buffer_;
  int content_length_;

  base::WeakPtrFactory<NodeDebugger> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeDebugger);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NODE_DEBUGGER_H_

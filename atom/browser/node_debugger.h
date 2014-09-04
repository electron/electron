// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NODE_DEBUGGER_H_
#define ATOM_BROWSER_NODE_DEBUGGER_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"

namespace net {
class StreamSocket;
class TCPServerSocket;
}

namespace atom {

// Add support for node's "--debug" switch.
class NodeDebugger {
 public:
  NodeDebugger();
  virtual ~NodeDebugger();

 private:
  void DoAcceptLoop();

  base::Thread thread_;
  scoped_ptr<net::TCPServerSocket> server_socket_;
  scoped_ptr<net::StreamSocket> accepted_socket_;

  base::WeakPtrFactory<NodeDebugger> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeDebugger);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NODE_DEBUGGER_H_

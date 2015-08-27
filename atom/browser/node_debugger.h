// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NODE_DEBUGGER_H_
#define ATOM_BROWSER_NODE_DEBUGGER_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "net/socket/stream_listen_socket.h"
#include "v8/include/v8-debug.h"

namespace atom {

// Add support for node's "--debug" switch.
class NodeDebugger : public net::StreamListenSocket::Delegate {
 public:
  explicit NodeDebugger(v8::Isolate* isolate);
  virtual ~NodeDebugger();

  bool IsRunning() const;

 private:
  void StartServer(int port);
  void CloseSession();
  void OnMessage(const std::string& message);
  void SendMessage(const std::string& message);
  void SendConnectMessage();

  static void DebugMessageHandler(const v8::Debug::Message& message);

  // net::StreamListenSocket::Delegate:
  void DidAccept(net::StreamListenSocket* server,
                 scoped_ptr<net::StreamListenSocket> socket) override;
  void DidRead(net::StreamListenSocket* socket,
               const char* data,
               int len) override;
  void DidClose(net::StreamListenSocket* socket) override;

  v8::Isolate* isolate_;

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

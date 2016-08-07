// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "net/test/embedded_test_server/stream_listen_socket.h"
#include "v8/include/v8-debug.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace atom {

// Add support for node's "--debug" switch.
class NodeDebugger : public net::test_server::StreamListenSocket::Delegate {
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

  static void ProcessMessageInUI(uv_async_t* handle);

  static void DebugMessageHandler(const v8::Debug::Message& message);

  // net::test_server::StreamListenSocket::Delegate:
  void DidAccept(
      net::test_server::StreamListenSocket* server,
      std::unique_ptr<net::test_server::StreamListenSocket> socket) override;
  void DidRead(net::test_server::StreamListenSocket* socket,
               const char* data,
               int len) override;
  void DidClose(net::test_server::StreamListenSocket* socket) override;

  v8::Isolate* isolate_;

  uv_async_t weak_up_ui_handle_;

  base::Thread thread_;
  std::unique_ptr<net::test_server::StreamListenSocket> server_;
  std::unique_ptr<net::test_server::StreamListenSocket> accepted_socket_;

  std::string buffer_;
  int content_length_;

  base::WeakPtrFactory<NodeDebugger> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeDebugger);
};

}  // namespace atom

// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/remote_debugging_server.h"

#include "browser/devtools_manager_delegate.h"

#include "base/files/file_util.h"
#include "content/public/browser/devtools_http_handler.h"
#include "net/socket/tcp_server_socket.h"

namespace brightray {

namespace {

class TCPServerSocketFactory
    : public content::DevToolsHttpHandler::ServerSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, uint16 port, int backlog)
      : content::DevToolsHttpHandler::ServerSocketFactory(address, port, backlog) {}

 private:
  // content::DevToolsHttpHandler::ServerSocketFactory:
  scoped_ptr<net::ServerSocket> Create() const override {
    return scoped_ptr<net::ServerSocket>(new net::TCPServerSocket(NULL, net::NetLog::Source()));
  }

  DISALLOW_COPY_AND_ASSIGN(TCPServerSocketFactory);
};

}  // namespace

RemoteDebuggingServer::RemoteDebuggingServer(const std::string& ip, uint16 port) {
  scoped_ptr<content::DevToolsHttpHandler::ServerSocketFactory> factory(
      new TCPServerSocketFactory(ip, port, 1));
  devtools_http_handler_ = DevToolsManagerDelegate::CreateHttpHandler();
}

RemoteDebuggingServer::~RemoteDebuggingServer() {
  devtools_http_handler_->Stop();
}

}  // namespace brightray

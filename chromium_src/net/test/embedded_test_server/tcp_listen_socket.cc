// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/embedded_test_server/tcp_listen_socket.h"

#if defined(OS_WIN)
// winsock2.h must be included first in order to ensure it is included before
// windows.h.
#include <winsock2.h>
#elif defined(OS_POSIX)
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "net/base/net_errors.h"
#endif

#include "base/logging.h"
#include "base/sys_byteorder.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "net/base/net_util.h"
#include "net/base/winsock_init.h"
#include "net/socket/socket_descriptor.h"

using std::string;

namespace net {

namespace test_server {

// static
scoped_ptr<TCPListenSocket> TCPListenSocket::CreateAndListen(
    const string& ip,
    uint16 port,
    StreamListenSocket::Delegate* del) {
  SocketDescriptor s = CreateAndBind(ip, port);
  if (s == kInvalidSocket)
    return scoped_ptr<TCPListenSocket>();
  scoped_ptr<TCPListenSocket> sock(new TCPListenSocket(s, del));
  sock->Listen();
  return sock.Pass();
}

TCPListenSocket::TCPListenSocket(SocketDescriptor s,
                                 StreamListenSocket::Delegate* del)
    : StreamListenSocket(s, del) {
}

TCPListenSocket::~TCPListenSocket() {
}

SocketDescriptor TCPListenSocket::CreateAndBind(const string& ip, uint16 port) {
  SocketDescriptor s = CreatePlatformSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s != kInvalidSocket) {
#if defined(OS_POSIX)
    // Allow rapid reuse.
    static const int kOn = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &kOn, sizeof(kOn));
#endif
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = base::HostToNet16(port);
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
#if defined(OS_WIN)
      closesocket(s);
#elif defined(OS_POSIX)
      close(s);
#endif
      LOG(ERROR) << "Could not bind socket to " << ip << ":" << port;
      s = kInvalidSocket;
    }
  }
  return s;
}

SocketDescriptor TCPListenSocket::CreateAndBindAnyPort(const string& ip,
                                                       uint16* port) {
  SocketDescriptor s = CreateAndBind(ip, 0);
  if (s == kInvalidSocket)
    return kInvalidSocket;
  sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  bool failed = getsockname(s, reinterpret_cast<struct sockaddr*>(&addr),
                            &addr_size) != 0;
  if (addr_size != sizeof(addr))
    failed = true;
  if (failed) {
    LOG(ERROR) << "Could not determine bound port, getsockname() failed";
#if defined(OS_WIN)
    closesocket(s);
#elif defined(OS_POSIX)
    close(s);
#endif
    return kInvalidSocket;
  }
  *port = base::NetToHost16(addr.sin_port);
  return s;
}

void TCPListenSocket::Accept() {
  SocketDescriptor conn = AcceptSocket();
  if (conn == kInvalidSocket)
    return;
  scoped_ptr<TCPListenSocket> sock(new TCPListenSocket(conn, socket_delegate_));
#if defined(OS_POSIX)
  sock->WatchSocket(WAITING_READ);
#endif
  socket_delegate_->DidAccept(this, sock.Pass());
}

}  // namespace test_server

}  // namespace net

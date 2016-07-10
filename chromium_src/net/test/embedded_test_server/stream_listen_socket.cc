// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/embedded_test_server/stream_listen_socket.h"

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

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/posix/eintr_wrapper.h"
#include "base/sys_byteorder.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/network_interfaces.h"
#include "net/base/sockaddr_storage.h"
#include "net/socket/socket_descriptor.h"

using std::string;

#if defined(OS_WIN)
typedef int socklen_t;
#endif  // defined(OS_WIN)

namespace net {

namespace test_server {

namespace {

const int kReadBufSize = 4096;

}  // namespace

#if defined(OS_WIN)
const int StreamListenSocket::kSocketError = SOCKET_ERROR;
#elif defined(OS_POSIX)
const int StreamListenSocket::kSocketError = -1;
#endif

StreamListenSocket::StreamListenSocket(SocketDescriptor s,
                                       StreamListenSocket::Delegate* del)
    : socket_delegate_(del),
      socket_(s),
      reads_paused_(false),
      has_pending_reads_(false) {
#if defined(OS_WIN)
  socket_event_ = WSACreateEvent();
  // TODO(ibrar): error handling in case of socket_event_ == WSA_INVALID_EVENT.
  WatchSocket(NOT_WAITING);
#elif defined(OS_POSIX)
  wait_state_ = NOT_WAITING;
#endif
}

StreamListenSocket::~StreamListenSocket() {
  CloseSocket();
#if defined(OS_WIN)
  if (socket_event_) {
    WSACloseEvent(socket_event_);
    socket_event_ = WSA_INVALID_EVENT;
  }
#endif
}

void StreamListenSocket::Send(const char* bytes,
                              int len,
                              bool append_linefeed) {
  SendInternal(bytes, len);
  if (append_linefeed)
    SendInternal("\r\n", 2);
}

void StreamListenSocket::Send(const string& str, bool append_linefeed) {
  Send(str.data(), static_cast<int>(str.length()), append_linefeed);
}

int StreamListenSocket::GetLocalAddress(IPEndPoint* address) const {
  SockaddrStorage storage;
  if (getsockname(socket_, storage.addr, &storage.addr_len)) {
#if defined(OS_WIN)
    int err = WSAGetLastError();
#else
    int err = errno;
#endif
    return MapSystemError(err);
  }
  if (!address->FromSockAddr(storage.addr, storage.addr_len))
    return ERR_ADDRESS_INVALID;
  return OK;
}

int StreamListenSocket::GetPeerAddress(IPEndPoint* address) const {
  SockaddrStorage storage;
  if (getpeername(socket_, storage.addr, &storage.addr_len)) {
#if defined(OS_WIN)
    int err = WSAGetLastError();
#else
    int err = errno;
#endif
    return MapSystemError(err);
  }

  if (!address->FromSockAddr(storage.addr, storage.addr_len))
    return ERR_ADDRESS_INVALID;

  return OK;
}

SocketDescriptor StreamListenSocket::AcceptSocket() {
  SocketDescriptor conn = HANDLE_EINTR(accept(socket_, nullptr, nullptr));
  if (conn == kInvalidSocket)
    LOG(ERROR) << "Error accepting connection.";
  else
    base::SetNonBlocking(conn);
  return conn;
}

void StreamListenSocket::SendInternal(const char* bytes, int len) {
  char* send_buf = const_cast<char*>(bytes);
  int len_left = len;
  while (true) {
    int sent = HANDLE_EINTR(send(socket_, send_buf, len_left, 0));
    if (sent == len_left) {  // A shortcut to avoid extraneous checks.
      break;
    }
    if (sent == kSocketError) {
#if defined(OS_WIN)
      if (WSAGetLastError() != WSAEWOULDBLOCK) {
        LOG(ERROR) << "send failed: WSAGetLastError()==" << WSAGetLastError();
#elif defined(OS_POSIX)
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        LOG(ERROR) << "send failed: errno==" << errno;
#endif
        break;
      }
      // Otherwise we would block, and now we have to wait for a retry.
      // Fall through to PlatformThread::YieldCurrentThread()
    } else {
      // sent != len_left according to the shortcut above.
      // Shift the buffer start and send the remainder after a short while.
      send_buf += sent;
      len_left -= sent;
    }
    base::PlatformThread::YieldCurrentThread();
  }
}

void StreamListenSocket::Listen() {
  int backlog = 10;  // TODO(erikkay): maybe don't allow any backlog?
  if (listen(socket_, backlog) == -1) {
    // TODO(erikkay): error handling.
    LOG(ERROR) << "Could not listen on socket.";
    return;
  }
#if defined(OS_POSIX)
  WatchSocket(WAITING_ACCEPT);
#endif
}

void StreamListenSocket::Read() {
  char buf[kReadBufSize + 1];  // +1 for null termination.
  int len;
  do {
    len = HANDLE_EINTR(recv(socket_, buf, kReadBufSize, 0));
    if (len == kSocketError) {
#if defined(OS_WIN)
      int err = WSAGetLastError();
      if (err == WSAEWOULDBLOCK) {
#elif defined(OS_POSIX)
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
#endif
        break;
      } else {
        // TODO(ibrar): some error handling required here.
        break;
      }
    } else if (len == 0) {
#if defined(OS_POSIX)
      // In Windows, Close() is called by OnObjectSignaled. In POSIX, we need
      // to call it here.
      Close();
#endif
    } else {
      // TODO(ibrar): maybe change DidRead to take a length instead.
      DCHECK_GT(len, 0);
      DCHECK_LE(len, kReadBufSize);
      buf[len] = 0;  // Already create a buffer with +1 length.
      socket_delegate_->DidRead(this, buf, len);
    }
  } while (len == kReadBufSize);
}

void StreamListenSocket::Close() {
#if defined(OS_POSIX)
  if (wait_state_ == NOT_WAITING)
    return;
  wait_state_ = NOT_WAITING;
#endif
  UnwatchSocket();
  socket_delegate_->DidClose(this);
}

void StreamListenSocket::CloseSocket() {
  if (socket_ != kInvalidSocket) {
    UnwatchSocket();
#if defined(OS_WIN)
    closesocket(socket_);
#elif defined(OS_POSIX)
    close(socket_);
#endif
  }
}

void StreamListenSocket::WatchSocket(WaitState state) {
#if defined(OS_WIN)
  WSAEventSelect(socket_, socket_event_, FD_ACCEPT | FD_CLOSE | FD_READ);
  watcher_.StartWatchingOnce(socket_event_, this);
#elif defined(OS_POSIX)
  // Implicitly calls StartWatchingFileDescriptor().
  base::MessageLoopForIO::current()->WatchFileDescriptor(
      socket_, true, base::MessageLoopForIO::WATCH_READ, &watcher_, this);
  wait_state_ = state;
#endif
}

void StreamListenSocket::UnwatchSocket() {
#if defined(OS_WIN)
  watcher_.StopWatching();
#elif defined(OS_POSIX)
  watcher_.StopWatchingFileDescriptor();
#endif
}

// TODO(ibrar): We can add these functions into OS dependent files.
#if defined(OS_WIN)
// MessageLoop watcher callback.
void StreamListenSocket::OnObjectSignaled(HANDLE object) {
  WSANETWORKEVENTS ev;
  if (kSocketError == WSAEnumNetworkEvents(socket_, socket_event_, &ev)) {
    // TODO
    return;
  }

  // If both FD_CLOSE and FD_READ are set we only call Read().
  // This will cause OnObjectSignaled to be called immediately again
  // unless this socket is destroyed in Read().
  if ((ev.lNetworkEvents & (FD_CLOSE | FD_READ)) == FD_CLOSE) {
    Close();
    // Close might have deleted this object. We should return immediately.
    return;
  }
  // The object was reset by WSAEnumNetworkEvents.  Watch for the next signal.
  watcher_.StartWatchingOnce(object, this);

  if (ev.lNetworkEvents == 0) {
    // Occasionally the event is set even though there is no new data.
    // The net seems to think that this is ignorable.
    return;
  }
  if (ev.lNetworkEvents & FD_ACCEPT) {
    Accept();
  }
  if (ev.lNetworkEvents & FD_READ) {
    if (reads_paused_) {
      has_pending_reads_ = true;
    } else {
      Read();
      // Read might have deleted this object. We should return immediately.
    }
  }
}
#elif defined(OS_POSIX)
void StreamListenSocket::OnFileCanReadWithoutBlocking(int fd) {
  switch (wait_state_) {
    case WAITING_ACCEPT:
      Accept();
      break;
    case WAITING_READ:
      if (reads_paused_) {
        has_pending_reads_ = true;
      } else {
        Read();
      }
      break;
    default:
      // Close() is called by Read() in the Linux case.
      NOTREACHED();
      break;
  }
}

void StreamListenSocket::OnFileCanWriteWithoutBlocking(int fd) {
  // MessagePumpLibevent callback, we don't listen for write events
  // so we shouldn't ever reach here.
  NOTREACHED();
}

#endif

void StreamListenSocket::PauseReads() {
  DCHECK(!reads_paused_);
  reads_paused_ = true;
}

void StreamListenSocket::ResumeReads() {
  DCHECK(reads_paused_);
  reads_paused_ = false;
  if (has_pending_reads_) {
    has_pending_reads_ = false;
    Read();
  }
}

}  // namespace test_server

}  // namespace net

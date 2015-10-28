// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Stream-based listen socket implementation that handles reading and writing
// to the socket, but does not handle creating the socket nor connecting
// sockets, which are handled by subclasses on creation and in Accept,
// respectively.

// StreamListenSocket handles IO asynchronously in the specified MessageLoop.
// This class is NOT thread safe. It uses WSAEVENT handles to monitor activity
// in a given MessageLoop. This means that callbacks will happen in that loop's
// thread always and that all other methods (including constructor and
// destructor) should also be called from the same thread.

#ifndef NET_TEST_EMBEDDED_TEST_SERVER_STREAM_LISTEN_SOCKET_H_
#define NET_TEST_EMBEDDED_TEST_SERVER_STREAM_LISTEN_SOCKET_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <winsock2.h>
#endif
#include <string>
#if defined(OS_WIN)
#include "base/win/object_watcher.h"
#elif defined(OS_POSIX)
#include "base/message_loop/message_loop.h"
#endif

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/net_export.h"
#include "net/socket/socket_descriptor.h"

namespace net {

class IPEndPoint;

namespace test_server {

class StreamListenSocket :
#if defined(OS_WIN)
    public base::win::ObjectWatcher::Delegate {
#elif defined(OS_POSIX)
    public base::MessageLoopForIO::Watcher {
#endif

 public:
  ~StreamListenSocket() override;

  // TODO(erikkay): this delegate should really be split into two parts
  // to split up the listener from the connected socket.  Perhaps this class
  // should be split up similarly.
  class Delegate {
   public:
    // |server| is the original listening Socket, connection is the new
    // Socket that was created.
    virtual void DidAccept(StreamListenSocket* server,
                           scoped_ptr<StreamListenSocket> connection) = 0;
    virtual void DidRead(StreamListenSocket* connection,
                         const char* data,
                         int len) = 0;
    virtual void DidClose(StreamListenSocket* sock) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // Send data to the socket.
  void Send(const char* bytes, int len, bool append_linefeed = false);
  void Send(const std::string& str, bool append_linefeed = false);

  // Copies the local address to |address|. Returns a network error code.
  // This method is virtual to support unit testing.
  virtual int GetLocalAddress(IPEndPoint* address) const;
  // Copies the peer address to |address|. Returns a network error code.
  // This method is virtual to support unit testing.
  virtual int GetPeerAddress(IPEndPoint* address) const;

  static const int kSocketError;

 protected:
  enum WaitState { NOT_WAITING = 0, WAITING_ACCEPT = 1, WAITING_READ = 2 };

  StreamListenSocket(SocketDescriptor s, Delegate* del);

  SocketDescriptor AcceptSocket();
  virtual void Accept() = 0;

  void Listen();
  void Read();
  void Close();
  void CloseSocket();

  // Pass any value in case of Windows, because in Windows
  // we are not using state.
  void WatchSocket(WaitState state);
  void UnwatchSocket();

  Delegate* const socket_delegate_;

 private:
  friend class TransportClientSocketTest;

  void SendInternal(const char* bytes, int len);

#if defined(OS_WIN)
  // ObjectWatcher delegate.
  void OnObjectSignaled(HANDLE object) override;
  base::win::ObjectWatcher watcher_;
  HANDLE socket_event_;
#elif defined(OS_POSIX)
  // Called by MessagePumpLibevent when the socket is ready to do I/O.
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;
  WaitState wait_state_;
  // The socket's libevent wrapper.
  base::MessageLoopForIO::FileDescriptorWatcher watcher_;
#endif

  // NOTE: This is for unit test use only!
  // Pause/Resume calling Read(). Note that ResumeReads() will also call
  // Read() if there is anything to read.
  void PauseReads();
  void ResumeReads();

  const SocketDescriptor socket_;
  bool reads_paused_;
  bool has_pending_reads_;

  DISALLOW_COPY_AND_ASSIGN(StreamListenSocket);
};

// Abstract factory that must be subclassed for each subclass of
// StreamListenSocket.
class StreamListenSocketFactory {
 public:
  virtual ~StreamListenSocketFactory() {}

  // Returns a new instance of StreamListenSocket or NULL if an error occurred.
  virtual scoped_ptr<StreamListenSocket> CreateAndListen(
      StreamListenSocket::Delegate* delegate) const = 0;
};

}  // namespace test_server

}  // namespace net

#endif  // NET_TEST_EMBEDDED_TEST_SERVER_STREAM_LISTEN_SOCKET_H_

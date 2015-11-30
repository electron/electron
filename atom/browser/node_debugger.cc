// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/node_debugger.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "net/test/embedded_test_server/tcp_listen_socket.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace {

// NodeDebugger is stored in Isolate's data, slots 0, 1, 3 have already been
// taken by gin, blink and node, using 2 is a safe option for now.
const int kIsolateSlot = 2;

const char* kContentLength = "Content-Length";

}  // namespace

NodeDebugger::NodeDebugger(v8::Isolate* isolate)
    : isolate_(isolate),
      thread_("NodeDebugger"),
      content_length_(-1),
      weak_factory_(this) {
  bool use_debug_agent = false;
  int port = 5858;

  std::string port_str;
  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch("debug")) {
    use_debug_agent = true;
    port_str = cmd->GetSwitchValueASCII("debug");
  } else if (cmd->HasSwitch("debug-brk")) {
    use_debug_agent = true;
    port_str = cmd->GetSwitchValueASCII("debug-brk");
  }

  if (use_debug_agent) {
    if (!port_str.empty())
      base::StringToInt(port_str, &port);

    isolate_->SetData(kIsolateSlot, this);
    v8::Debug::SetMessageHandler(DebugMessageHandler);

    uv_async_init(uv_default_loop(), &weak_up_ui_handle_, ProcessMessageInUI);

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

bool NodeDebugger::IsRunning() const {
  return thread_.IsRunning();
}

void NodeDebugger::StartServer(int port) {
  server_ = net::test_server::TCPListenSocket::CreateAndListen(
      "127.0.0.1", port, this);
  if (!server_) {
    LOG(ERROR) << "Cannot start debugger server";
    return;
  }
}

void NodeDebugger::CloseSession() {
  accepted_socket_.reset();
}

void NodeDebugger::OnMessage(const std::string& message) {
  if (message.find("\"type\":\"request\",\"command\":\"disconnect\"}") !=
          std::string::npos)
    CloseSession();

  base::string16 message16 = base::UTF8ToUTF16(message);
  v8::Debug::SendCommand(
      isolate_,
      reinterpret_cast<const uint16_t*>(message16.data()), message16.size());

  uv_async_send(&weak_up_ui_handle_);
}

void NodeDebugger::SendMessage(const std::string& message) {
  if (accepted_socket_) {
    std::string header = base::StringPrintf(
        "%s: %d\r\n\r\n", kContentLength, static_cast<int>(message.size()));
    accepted_socket_->Send(header);
    accepted_socket_->Send(message);
  }
}

void NodeDebugger::SendConnectMessage() {
  accepted_socket_->Send(base::StringPrintf(
      "Type: connect\r\n"
      "V8-Version: %s\r\n"
      "Protocol-Version: 1\r\n"
      "Embedding-Host: %s\r\n"
      "%s: 0\r\n",
      v8::V8::GetVersion(), ATOM_PRODUCT_NAME, kContentLength), true);
}

// static
void NodeDebugger::ProcessMessageInUI(uv_async_t* handle) {
  v8::Debug::ProcessDebugMessages();
}

// static
void NodeDebugger::DebugMessageHandler(const v8::Debug::Message& message) {
  NodeDebugger* self = static_cast<NodeDebugger*>(
      message.GetIsolate()->GetData(kIsolateSlot));

  if (self) {
    std::string message8(*v8::String::Utf8Value(message.GetJSON()));
    self->thread_.message_loop()->PostTask(
        FROM_HERE,
        base::Bind(&NodeDebugger::SendMessage, self->weak_factory_.GetWeakPtr(),
                   message8));
  }
}

void NodeDebugger::DidAccept(
    net::test_server::StreamListenSocket* server,
    scoped_ptr<net::test_server::StreamListenSocket> socket) {
  // Only accept one session.
  if (accepted_socket_) {
    socket->Send(std::string("Remote debugging session already active"), true);
    return;
  }

  accepted_socket_ = socket.Pass();
  SendConnectMessage();
}

void NodeDebugger::DidRead(net::test_server::StreamListenSocket* socket,
                           const char* data,
                           int len) {
  buffer_.append(data, len);

  do {
    if (buffer_.size() == 0)
      return;

    // Read the "Content-Length" header.
    if (content_length_ < 0) {
      size_t pos = buffer_.find("\r\n\r\n");
      if (pos == std::string::npos)
        return;

      // We can be sure that the header is "Content-Length: xxx\r\n".
      std::string content_length = buffer_.substr(16, pos - 16);
      if (!base::StringToInt(content_length, &content_length_)) {
        DidClose(accepted_socket_.get());
        return;
      }

      // Strip header from buffer.
      buffer_ = buffer_.substr(pos + 4);
    }

    // Read the message.
    if (buffer_.size() >= static_cast<size_t>(content_length_)) {
      std::string message = buffer_.substr(0, content_length_);
      buffer_ = buffer_.substr(content_length_);

      OnMessage(message);

      // Get ready for next message.
      content_length_ = -1;
    }
  } while (true);
}

void NodeDebugger::DidClose(net::test_server::StreamListenSocket* socket) {
  // If we lost the connection, then simulate a disconnect msg:
  OnMessage("{\"seq\":1,\"type\":\"request\",\"command\":\"disconnect\"}");
}

}  // namespace atom

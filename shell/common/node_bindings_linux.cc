// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_bindings_linux.h"

#include <sys/epoll.h>

namespace electron {

NodeBindingsLinux::NodeBindingsLinux(BrowserEnvironment browser_env)
    : NodeBindings(browser_env), epoll_(epoll_create(1)) {
  int backend_fd = uv_backend_fd(uv_loop_);
  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.fd = backend_fd;
  epoll_ctl(epoll_, EPOLL_CTL_ADD, backend_fd, &ev);
}

NodeBindingsLinux::~NodeBindingsLinux() = default;

void NodeBindingsLinux::PrepareMessageLoop() {
  int handle = uv_backend_fd(uv_loop_);

  // If the backend fd hasn't changed, don't proceed.
  if (handle == handle_)
    return;

  NodeBindings::PrepareMessageLoop();
}

void NodeBindingsLinux::RunMessageLoop() {
  int handle = uv_backend_fd(uv_loop_);

  // If the backend fd hasn't changed, don't proceed.
  if (handle == handle_)
    return;

  handle_ = handle;

  NodeBindings::RunMessageLoop();
}

void NodeBindingsLinux::PollEvents() {
  int timeout = uv_backend_timeout(uv_loop_);

  // Wait for new libuv events.
  int r;
  do {
    struct epoll_event ev;
    r = epoll_wait(epoll_, &ev, 1, timeout);
  } while (r == -1 && errno == EINTR);
}

// static
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsLinux(browser_env);
}

}  // namespace electron

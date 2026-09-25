// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_bindings_linux.h"

#include <sys/epoll.h>

namespace electron {

NodeBindingsLinux::NodeBindingsLinux(BrowserEnvironment browser_env,
                                     uv_loop_t* loop)
    : NodeBindings(browser_env, loop), epoll_(epoll_create(1)) {
  auto* const event_loop = uv_loop();

  int backend_fd = uv_backend_fd(event_loop);
  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.fd = backend_fd;
  epoll_ctl(epoll_.get(), EPOLL_CTL_ADD, backend_fd, &ev);
}

// The embed thread uses epoll_, so it has to be gone before the members are.
NodeBindingsLinux::~NodeBindingsLinux() {
  StopPolling();
}

void NodeBindingsLinux::PollEvents(int timeout) {
  int r;
  do {
    struct epoll_event ev;
    r = epoll_wait(epoll_.get(), &ev, 1, timeout);
  } while (r == -1 && errno == EINTR);
}

// static
std::unique_ptr<NodeBindings> NodeBindings::Create(BrowserEnvironment env,
                                                   uv_loop_t* loop) {
  return std::make_unique<NodeBindingsLinux>(env, loop);
}

}  // namespace electron

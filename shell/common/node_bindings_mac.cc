// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_bindings_mac.h"

#include <errno.h>
#include <poll.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>

namespace electron {

NodeBindingsMac::NodeBindingsMac(BrowserEnvironment browser_env)
    : NodeBindings(browser_env) {}

void NodeBindingsMac::PollEvents() {
  auto* const event_loop = uv_loop();
  // uv_backend_timeout returns milliseconds or -1 for infinite wait.
  const int backend_fd = uv_backend_fd(event_loop);
  const int timeout_ms = uv_backend_timeout(event_loop);  // -1 => infinite

  struct pollfd pfd;
  pfd.fd = backend_fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  int r;
  do {
    r = poll(&pfd, 1, timeout_ms);
  } while (r == -1 && errno == EINTR);
}

// static
std::unique_ptr<NodeBindings> NodeBindings::Create(BrowserEnvironment env) {
  return std::make_unique<NodeBindingsMac>(env);
}

}  // namespace electron

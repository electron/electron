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

NodeBindingsMac::NodeBindingsMac(BrowserEnvironment browser_env,
                                 uv_loop_t* loop)
    : NodeBindings(browser_env, loop) {}

void NodeBindingsMac::PollEvents(int timeout) {
  struct pollfd pfd;
  pfd.fd = uv_backend_fd(uv_loop());
  pfd.events = POLLIN;
  pfd.revents = 0;

  int r;
  do {
    r = poll(&pfd, 1, timeout);
  } while (r == -1 && errno == EINTR);
}

// static
std::unique_ptr<NodeBindings> NodeBindings::Create(BrowserEnvironment env,
                                                   uv_loop_t* loop) {
  return std::make_unique<NodeBindingsMac>(env, loop);
}

}  // namespace electron

// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/node_bindings_mac.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "shell/common/node_includes.h"

namespace electron {

NodeBindingsMac::NodeBindingsMac(BrowserEnvironment browser_env)
    : NodeBindings(browser_env) {}

NodeBindingsMac::~NodeBindingsMac() = default;

void NodeBindingsMac::PrepareMessageLoop() {
  int handle = uv_backend_fd(uv_loop_);

  // If the backend fd hasn't changed, don't proceed.
  if (handle == handle_)
    return;

  NodeBindings::PrepareMessageLoop();
}

void NodeBindingsMac::RunMessageLoop() {
  int handle = uv_backend_fd(uv_loop_);

  // If the backend fd hasn't changed, don't proceed.
  if (handle == handle_)
    return;

  handle_ = handle;

  NodeBindings::RunMessageLoop();
}

void NodeBindingsMac::PollEvents() {
  struct timeval tv;
  int timeout = uv_backend_timeout(uv_loop_);
  if (timeout != -1) {
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
  }

  fd_set readset;
  int fd = uv_backend_fd(uv_loop_);
  FD_ZERO(&readset);
  FD_SET(fd, &readset);

  // Wait for new libuv events.
  int r;
  do {
    r = select(fd + 1, &readset, nullptr, nullptr,
               timeout == -1 ? nullptr : &tv);
  } while (r == -1 && errno == EINTR);
}

// static
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsMac(browser_env);
}

}  // namespace electron

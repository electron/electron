// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/node_bindings_mac.h"

#include <errno.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "atom/common/node_includes.h"

namespace atom {

NodeBindingsMac::NodeBindingsMac(bool is_browser)
    : NodeBindings(is_browser),
      kqueue_(kqueue()) {
  // Add uv's backend fd to kqueue.
  struct kevent ev;
  EV_SET(&ev, uv_backend_fd(uv_loop_), EVFILT_READ, EV_ADD | EV_ENABLE,
         0, 0, 0);
  kevent(kqueue_, &ev, 1, NULL, 0, NULL);
}

NodeBindingsMac::~NodeBindingsMac() {
}

void NodeBindingsMac::RunMessageLoop() {
  // Get notified when libuv's watcher queue changes.
  uv_loop_->data = this;
  uv_loop_->on_watcher_queue_updated = OnWatcherQueueChanged;

  NodeBindings::RunMessageLoop();
}

// static
void NodeBindingsMac::OnWatcherQueueChanged(uv_loop_t* loop) {
  NodeBindingsMac* self = static_cast<NodeBindingsMac*>(loop->data);

  // We need to break the io polling in the kqueue thread when loop's watcher
  // queue changes, otherwise new events cannot be notified.
  self->WakeupEmbedThread();
}

void NodeBindingsMac::PollEvents() {
  struct timespec spec;
  int timeout = uv_backend_timeout(uv_loop_);
  if (timeout != -1) {
    spec.tv_sec = timeout / 1000;
    spec.tv_nsec = (timeout % 1000) * 1000000;
  }

  // Wait for new libuv events.
  int r;
  do {
    struct kevent ev;
    r = ::kevent(kqueue_, NULL, 0, &ev, 1,
                 timeout == -1 ? NULL : &spec);
  } while (r == -1 && errno == EINTR);
}

// static
NodeBindings* NodeBindings::Create(bool is_browser) {
  return new NodeBindingsMac(is_browser);
}

}  // namespace atom

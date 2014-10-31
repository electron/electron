// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/node_bindings_win.h"

#include <windows.h>

#include "base/logging.h"

extern "C" {
#include "vendor/node/deps/uv/src/win/internal.h"
}

namespace atom {

NodeBindingsWin::NodeBindingsWin(bool is_browser)
    : NodeBindings(is_browser) {
}

NodeBindingsWin::~NodeBindingsWin() {
}

void NodeBindingsWin::PollEvents() {
  // Unlike Unix, in which we can just rely on one backend fd to determine
  // whether we should iterate libuv loop, on Window, IOCP is just one part
  // of the libuv loop, we should also check whether we have other types of
  // events.
  bool block = uv_loop_->idle_handles == NULL &&
               uv_loop_->pending_reqs_tail == NULL &&
               uv_loop_->endgame_handles == NULL &&
               !uv_loop_->stop_flag &&
               (uv_loop_->active_handles > 0 ||
                !QUEUE_EMPTY(&uv_loop_->active_reqs));

  // When there is no other types of events, we block on the IOCP.
  if (block) {
    DWORD bytes, timeout;
    ULONG_PTR key;
    OVERLAPPED* overlapped;

    timeout = uv_backend_timeout(uv_loop_);
    GetQueuedCompletionStatus(uv_loop_->iocp,
                              &bytes,
                              &key,
                              &overlapped,
                              timeout);

    // Give the event back so libuv can deal with it.
    if (overlapped != NULL)
      PostQueuedCompletionStatus(uv_loop_->iocp,
                                 bytes,
                                 key,
                                 overlapped);
  }
}

// static
NodeBindings* NodeBindings::Create(bool is_browser) {
  return new NodeBindingsWin(is_browser);
}

}  // namespace atom

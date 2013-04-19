// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_BINDINGS_MAC_
#define ATOM_COMMON_NODE_BINDINGS_MAC_

#include "base/compiler_specific.h"
#include "common/node_bindings.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace atom {

class NodeBindingsMac : public NodeBindings {
 public:
  explicit NodeBindingsMac(bool is_browser);
  virtual ~NodeBindingsMac();

  virtual void PrepareMessageLoop() OVERRIDE;
  virtual void RunMessageLoop() OVERRIDE;

 private:
  // Run the libuv loop for once.
  void UvRunOnce();

  // Thread to poll uv events.
  static void EmbedThreadRunner(void *arg);

  // Called when uv's watcher queue changes.
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  // Main thread's loop.
  uv_loop_t* loop_;

  // Kqueue to poll for uv's backend fd.
  int kqueue_;

  // Dummy handle to make uv's loop not quit.
  uv_async_t dummy_uv_handle_;

  // Thread for polling events.
  uv_thread_t embed_thread_;

  // Whether we're done.
  bool embed_closed_;

  // Semaphore to wait for main loop in the embed thread.
  uv_sem_t embed_sem_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsMac);
};

}  // namespace atom

#endif  // ATOM_COMMON_NODE_BINDINGS_MAC_

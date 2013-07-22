// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RAVE_COMMON_NODE_BINDINGS_
#define RAVE_COMMON_NODE_BINDINGS_

#include "base/basictypes.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace WebKit {
class WebFrame;
}

namespace base {
class MessageLoop;
}

namespace atom {

class NodeBindings {
 public:
  static NodeBindings* CreateInBrowser();
  static NodeBindings* CreateInRenderer();

  virtual ~NodeBindings();

  // Setup V8, libuv and the process object.
  virtual void Initialize();

  // Load node.js main script.
  virtual void Load();

  // Load cefode.js script under web frame.
  virtual void BindTo(WebKit::WebFrame* frame);

  // Prepare for message loop integration.
  virtual void PrepareMessageLoop();

  // Do message loop integration.
  virtual void RunMessageLoop();

 protected:
  explicit NodeBindings(bool is_browser);

  // Called to poll events in new thread.
  virtual void PollEvents() = 0;

  // Run the libuv loop for once.
  void UvRunOnce();

  // Make the main thread run libuv loop.
  void WakeupMainThread();

  // Are we running in browser.
  bool is_browser_;

  // Main thread's MessageLoop.
  base::MessageLoop* message_loop_;

  // Main thread's libuv loop.
  uv_loop_t* uv_loop_;

 private:
  // Thread to poll uv events.
  static void EmbedThreadRunner(void *arg);

  // Called when uv's watcher queue changes.
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  // Whether the libuv loop has ended.
  bool embed_closed_;

  // Dummy handle to make uv's loop not quit.
  uv_async_t dummy_uv_handle_;

  // Thread for polling events.
  uv_thread_t embed_thread_;

  // Semaphore to wait for main loop in the embed thread.
  uv_sem_t embed_sem_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindings);
};

}  // namespace atom

#endif  // RAVE_COMMON_NODE_BINDINGS_

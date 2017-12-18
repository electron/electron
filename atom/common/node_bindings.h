// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_BINDINGS_H_
#define ATOM_COMMON_NODE_BINDINGS_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "v8/include/v8.h"
#include "vendor/node/deps/uv/include/uv.h"

namespace base {
class MessageLoop;
}

namespace node {
class Environment;
}

namespace atom {

class NodeBindings {
 public:
  enum BrowserEnvironment {
    BROWSER,
    RENDERER,
    WORKER,
  };

  static NodeBindings* Create(BrowserEnvironment browser_env);

  virtual ~NodeBindings();

  // Setup V8, libuv.
  void Initialize();

  // Create the environment and load node.js.
  node::Environment* CreateEnvironment(v8::Handle<v8::Context> context);

  // Load node.js in the environment.
  void LoadEnvironment(node::Environment* env);

  // Prepare for message loop integration.
  void PrepareMessageLoop();

  // Do message loop integration.
  virtual void RunMessageLoop();

  // Gets/sets the environment to wrap uv loop.
  void set_uv_env(node::Environment* env) { uv_env_ = env; }
  node::Environment* uv_env() const { return uv_env_; }

  uv_loop_t* uv_loop() const { return uv_loop_; }

 protected:
  explicit NodeBindings(BrowserEnvironment browser_env);

  // Called to poll events in new thread.
  virtual void PollEvents() = 0;

  // Run the libuv loop for once.
  void UvRunOnce();

  // Make the main thread run libuv loop.
  void WakeupMainThread();

  // Interrupt the PollEvents.
  void WakeupEmbedThread();

  // Which environment we are running.
  BrowserEnvironment browser_env_;

  // Current thread's MessageLoop.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Current thread's libuv loop.
  uv_loop_t* uv_loop_;

 private:
  // Thread to poll uv events.
  static void EmbedThreadRunner(void *arg);

  // Whether the libuv loop has ended.
  bool embed_closed_;

  // Loop used when constructed in WORKER mode
  uv_loop_t worker_loop_;

  // Dummy handle to make uv's loop not quit.
  uv_async_t dummy_uv_handle_;

  // Thread for polling events.
  uv_thread_t embed_thread_;

  // Semaphore to wait for main loop in the embed thread.
  uv_sem_t embed_sem_;

  // Environment that to wrap the uv loop.
  node::Environment* uv_env_;

  base::WeakPtrFactory<NodeBindings> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindings);
};

}  // namespace atom

#endif  // ATOM_COMMON_NODE_BINDINGS_H_

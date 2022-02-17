// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_BINDINGS_H_
#define ELECTRON_SHELL_COMMON_NODE_BINDINGS_H_

#include <type_traits>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "uv.h"  // NOLINT(build/include_directory)
#include "v8/include/v8.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace node {
class Environment;
class MultiIsolatePlatform;
class IsolateData;
}  // namespace node

namespace electron {

// A helper class to manage uv_handle_t types, e.g. uv_async_t.
//
// As per the uv docs: "uv_close() MUST be called on each handle before
// memory is released. Moreover, the memory can only be released in
// close_cb or after it has returned." This class encapsulates the work
// needed to follow those requirements.
template <typename T,
          typename std::enable_if<
              // these are the C-style 'subclasses' of uv_handle_t
              std::is_same<T, uv_async_t>::value ||
              std::is_same<T, uv_check_t>::value ||
              std::is_same<T, uv_fs_event_t>::value ||
              std::is_same<T, uv_fs_poll_t>::value ||
              std::is_same<T, uv_idle_t>::value ||
              std::is_same<T, uv_pipe_t>::value ||
              std::is_same<T, uv_poll_t>::value ||
              std::is_same<T, uv_prepare_t>::value ||
              std::is_same<T, uv_process_t>::value ||
              std::is_same<T, uv_signal_t>::value ||
              std::is_same<T, uv_stream_t>::value ||
              std::is_same<T, uv_tcp_t>::value ||
              std::is_same<T, uv_timer_t>::value ||
              std::is_same<T, uv_tty_t>::value ||
              std::is_same<T, uv_udp_t>::value>::type* = nullptr>
class UvHandle {
 public:
  UvHandle() : t_(new T) {}
  ~UvHandle() { reset(); }
  T* get() { return t_; }
  uv_handle_t* handle() { return reinterpret_cast<uv_handle_t*>(t_); }

  void reset() {
    auto* h = handle();
    if (h != nullptr) {
      DCHECK_EQ(0, uv_is_closing(h));
      uv_close(h, OnClosed);
      t_ = nullptr;
    }
  }

 private:
  static void OnClosed(uv_handle_t* handle) {
    delete reinterpret_cast<T*>(handle);
  }

  T* t_ = {};
};

class NodeBindings {
 public:
  enum class BrowserEnvironment { kBrowser, kRenderer, kWorker };

  static NodeBindings* Create(BrowserEnvironment browser_env);
  static void RegisterBuiltinModules();
  static bool IsInitialized();

  virtual ~NodeBindings();

  // Setup V8, libuv.
  void Initialize();

  // Create the environment and load node.js.
  node::Environment* CreateEnvironment(v8::Handle<v8::Context> context,
                                       node::MultiIsolatePlatform* platform);

  // Load node.js in the environment.
  void LoadEnvironment(node::Environment* env);

  // Prepare for message loop integration.
  void PrepareMessageLoop();

  // Do message loop integration.
  virtual void RunMessageLoop();

  node::IsolateData* isolate_data() const { return isolate_data_; }

  // Gets/sets the environment to wrap uv loop.
  void set_uv_env(node::Environment* env) { uv_env_ = env; }
  node::Environment* uv_env() const { return uv_env_; }

  uv_loop_t* uv_loop() const { return uv_loop_; }

  bool in_worker_loop() const { return uv_loop_ == &worker_loop_; }

  // disable copy
  NodeBindings(const NodeBindings&) = delete;
  NodeBindings& operator=(const NodeBindings&) = delete;

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
  const BrowserEnvironment browser_env_;

  // Current thread's MessageLoop.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Current thread's libuv loop.
  uv_loop_t* uv_loop_;

 private:
  // Thread to poll uv events.
  static void EmbedThreadRunner(void* arg);

  // Whether the libuv loop has ended.
  bool embed_closed_ = false;

  // Loop used when constructed in WORKER mode
  uv_loop_t worker_loop_;

  // Dummy handle to make uv's loop not quit.
  UvHandle<uv_async_t> dummy_uv_handle_;

  // Thread for polling events.
  uv_thread_t embed_thread_;

  // Semaphore to wait for main loop in the embed thread.
  uv_sem_t embed_sem_;

  // Environment that to wrap the uv loop.
  node::Environment* uv_env_ = nullptr;

  // Isolate data used in creating the environment
  node::IsolateData* isolate_data_ = nullptr;

#if !BUILDFLAG(IS_WIN)
  int handle_ = -1;
#endif

  base::WeakPtrFactory<NodeBindings> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_NODE_BINDINGS_H_

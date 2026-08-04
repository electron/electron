// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ELECTRON_SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>

#include "shell/common/uv_includes.h"
#include "third_party/electron_node/src/tracing/agent.h"

namespace gin {
class IsolateHolder;
}  // namespace gin

namespace node {
class Environment;
class MultiIsolatePlatform;
struct SnapshotData;
}  // namespace node

namespace v8 {
class Isolate;
class Locker;
}  // namespace v8

namespace electron {

class MicrotasksRunner;
// Manage the V8 isolate and context automatically.
class JavascriptEnvironment {
 public:
  JavascriptEnvironment(uv_loop_t* event_loop,
                        bool setup_wasm_streaming = false);
  ~JavascriptEnvironment();

  // disable copy
  JavascriptEnvironment(const JavascriptEnvironment&) = delete;
  JavascriptEnvironment& operator=(const JavascriptEnvironment&) = delete;

  void CreateMicrotasksRunner();
  void DestroyMicrotasksRunner();

  node::MultiIsolatePlatform* platform() const { return platform_.get(); }

  size_t max_young_generation_size_in_bytes() const {
    return max_young_generation_size_;
  }

  [[nodiscard]] v8::Isolate* isolate() const;
  [[nodiscard]] static v8::Isolate* GetIsolate();

  // The embedded Node.js startup snapshot this process's JavascriptEnvironment
  // isolate is (to be) created from, or nullptr when the Node.js environment
  // is bootstrapped from scratch: on builds without a Node snapshot
  // (cross-compiled targets), when a custom V8 snapshot is loaded, and in
  // process types that never have a JavascriptEnvironment. When non-null the
  // constructor creates no context; the main context comes out of
  // node::CreateEnvironment (pass it an empty one) and the caller enters it.
  [[nodiscard]] static const node::SnapshotData* NodeSnapshot();

 private:
  v8::Isolate* Initialize(uv_loop_t* event_loop, bool setup_wasm_streaming);
  std::unique_ptr<node::tracing::Agent, node::tracing::Agent::Deleter>
      tracing_agent_;
  std::unique_ptr<node::MultiIsolatePlatform> platform_;

  size_t max_young_generation_size_ = 0;
  std::unique_ptr<gin::IsolateHolder> isolate_holder_;

  // depends-on: isolate_holder_'s isolate
  std::unique_ptr<v8::Locker> locker_;

  std::unique_ptr<MicrotasksRunner> microtasks_runner_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

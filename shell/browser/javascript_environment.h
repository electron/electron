// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>

#include "base/macros.h"
#include "gin/public/isolate_holder.h"
#include "uv.h"  // NOLINT(build/include_directory)

namespace node {
class Environment;
class MultiIsolatePlatform;
}  // namespace node

namespace electron {

class MicrotasksRunner;
// Manage the V8 isolate and context automatically.
class JavascriptEnvironment {
 public:
  explicit JavascriptEnvironment(uv_loop_t* event_loop);
  ~JavascriptEnvironment();

  void OnMessageLoopCreated();
  void OnMessageLoopDestroying();

  node::MultiIsolatePlatform* platform() const { return platform_; }
  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  static v8::Isolate* GetIsolate();

 private:
  v8::Isolate* Initialize(uv_loop_t* event_loop);
  // Leaked on exit.
  node::MultiIsolatePlatform* platform_;

  v8::Isolate* isolate_;
  gin::IsolateHolder isolate_holder_;
  v8::Locker locker_;
  v8::Global<v8::Context> context_;

  std::unique_ptr<MicrotasksRunner> microtasks_runner_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptEnvironment);
};

// Manage the Node Environment automatically.
class NodeEnvironment {
 public:
  explicit NodeEnvironment(node::Environment* env);
  ~NodeEnvironment();

  node::Environment* env() { return env_; }

 private:
  node::Environment* env_;

  DISALLOW_COPY_AND_ASSIGN(NodeEnvironment);
};

}  // namespace electron

#endif  // SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

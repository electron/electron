// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include "base/macros.h"
#include "gin/public/isolate_holder.h"
#include "uv.h"  // NOLINT(build/include)

namespace node {
class Environment;
class MultiIsolatePlatform;
}  // namespace node

namespace atom {

// Manage the V8 isolate and context automatically.
class JavascriptEnvironment {
 public:
  explicit JavascriptEnvironment(uv_loop_t* event_loop);
  ~JavascriptEnvironment();

  void OnMessageLoopDestroying();

  node::MultiIsolatePlatform* platform() const { return platform_; }
  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

 private:
  v8::Isolate* Initialize(uv_loop_t* event_loop);
  // Leaked on exit.
  node::MultiIsolatePlatform* platform_;

  v8::Isolate* isolate_;
  gin::IsolateHolder isolate_holder_;
  v8::Isolate::Scope isolate_scope_;
  v8::Locker locker_;
  v8::HandleScope handle_scope_;
  v8::Global<v8::Context> context_;
  v8::Context::Scope context_scope_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptEnvironment);
};

// Manage the Node Environment automatically.
class NodeEnvironment {
 public:
  explicit NodeEnvironment(node::Environment* env);
  ~NodeEnvironment();

 private:
  node::Environment* env_;

  DISALLOW_COPY_AND_ASSIGN(NodeEnvironment);
};

}  // namespace atom

#endif  // ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

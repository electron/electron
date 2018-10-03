// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include "base/macros.h"
#include "gin/public/isolate_holder.h"

namespace node {
class Environment;
class MultiIsolatePlatform;
class IsolateData;
}  // namespace node

namespace atom {

// Manage the V8 isolate and context automatically.
class JavascriptEnvironment {
 public:
  JavascriptEnvironment();
  ~JavascriptEnvironment();

  void OnMessageLoopCreated();
  void OnMessageLoopDestroying();

  node::MultiIsolatePlatform* platform() const { return platform_; }
  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }
  void set_isolate_data(node::IsolateData* data) { isolate_data_ = data; }

 private:
  bool Initialize();

  // Leaked on exit.
  node::MultiIsolatePlatform* platform_;

  bool initialized_;
  gin::IsolateHolder isolate_holder_;
  v8::Isolate* isolate_;
  v8::Isolate::Scope isolate_scope_;
  v8::Locker locker_;
  v8::HandleScope handle_scope_;
  v8::UniquePersistent<v8::Context> context_;
  v8::Context::Scope context_scope_;
  node::IsolateData* isolate_data_;

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

class AtomIsolateAllocator : public gin::IsolateAllocater {
 public:
  explicit AtomIsolateAllocator(JavascriptEnvironment* env);

  v8::Isolate* Allocate() override;

 private:
  JavascriptEnvironment* env_;

  DISALLOW_COPY_AND_ASSIGN(AtomIsolateAllocator);
};

}  // namespace atom

#endif  // ATOM_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

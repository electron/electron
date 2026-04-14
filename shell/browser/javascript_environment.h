// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define ELECTRON_SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>

#include "uv.h"  // NOLINT(build/include_directory)

namespace gin {
class IsolateHolder;
}  // namespace gin

namespace node {
class Environment;
class MultiIsolatePlatform;
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

 private:
  v8::Isolate* Initialize(uv_loop_t* event_loop, bool setup_wasm_streaming);
  std::unique_ptr<node::MultiIsolatePlatform> platform_;

  size_t max_young_generation_size_ = 0;
  std::unique_ptr<gin::IsolateHolder> isolate_holder_;

  // depends-on: isolate_holder_'s isolate
  std::unique_ptr<v8::Locker> locker_;

  std::unique_ptr<MicrotasksRunner> microtasks_runner_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

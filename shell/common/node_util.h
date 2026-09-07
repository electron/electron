// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_UTIL_H_
#define ELECTRON_SHELL_COMMON_NODE_UTIL_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "v8-microtask-queue.h"
#include "v8/include/cppgc/macros.h"
#include "v8/include/v8-forward.h"

namespace node {
class Environment;
class IsolateData;

namespace EnvironmentFlags {
enum Flags : uint64_t;
}
}  // namespace node

namespace electron::util {

// Emit a warning via node's process.emitWarning()
void EmitWarning(v8::Isolate* isolate,
                 std::string_view warning_msg,
                 std::string_view warning_type);

// Emit a warning via node's process.emitWarning(),
// using JavascriptEnvironment's isolate
void EmitWarning(std::string_view warning_msg, std::string_view warning_type);

// Emit a deprecation warning via node's process.emitWarning()
void EmitDeprecationWarning(v8::Isolate* isolate,
                            std::string_view warning_msg,
                            std::string_view deprecation_code = "");

// Emit a deprecation warning via node's process.emitWarning(),
// using JavascriptEnvironment's isolate
void EmitDeprecationWarning(std::string_view warning_msg,
                            std::string_view deprecation_code = "");

// Compiles one of the electron/js2c/* bundles (with the build-time code cache)
// into a function taking `parameters`; logs and returns empty on failure.
v8::MaybeLocal<v8::Function> CompileBundle(
    v8::Local<v8::Context> context,
    const char* id,
    v8::LocalVector<v8::String>* parameters);

// Run a script with JS source bundled inside the binary as if it's wrapped
// in a function called with a null receiver and arguments specified in C++.
// The returned value is empty if an exception is encountered; the exception
// is caught and logged. JS code run with this method can assume that their
// top-level declarations won't affect the global scope.
v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    const char* id,
    v8::LocalVector<v8::String>* parameters,
    v8::LocalVector<v8::Value>* arguments);

// Feeds the build-time js2c code cache to the Environment's BuiltinLoader
// so the *_init bundles are consumed. Call once, after CreateEnvironment and
// before LoadEnvironment.
void FeedEnvironmentCodeCache(node::Environment* env);

// Install this process's build-time code cache (see node_natives_code_cache.h)
// as the default every node BuiltinLoader constructed from now on starts with,
// so builtins compiled before Electron can reach an Environment's loader --
// the per-context scripts node::NewContext runs and the whole
// internal/bootstrap/* sequence inside node::CreateEnvironment -- consume it
// too. Idempotent; call before the first node::NewContext / CreateEnvironment
// in a process that hosts a Node.js environment.
void InstallProcessCodeCache();

// Wrapper for node::CreateEnvironment that logs failure
node::Environment* CreateEnvironment(v8::Isolate* isolate,
                                     node::IsolateData* isolate_data,
                                     v8::Local<v8::Context> context,
                                     const std::vector<std::string>& args,
                                     const std::vector<std::string>& exec_args,
                                     node::EnvironmentFlags::Flags env_flags,
                                     std::string_view process_type = "");

v8::Local<v8::Object> CreateAbortController(v8::Isolate* isolate);

// A scope that temporarily changes the microtask policy to explicit. Use this
// anywhere that can trigger Node.js or uv_run().
//
// Node.js expects `kExplicit` microtasks policy and will run microtasks
// checkpoints after every call into JavaScript. Since we use a different
// policy in the renderer, this scope temporarily changes the policy to
// `kExplicit` while the scope is active, then restores the original policy
// when it's destroyed.
class ExplicitMicrotasksScope {
  CPPGC_STACK_ALLOCATED();

 public:
  explicit ExplicitMicrotasksScope(v8::MicrotaskQueue* queue);
  ~ExplicitMicrotasksScope();

  ExplicitMicrotasksScope(const ExplicitMicrotasksScope&) = delete;
  ExplicitMicrotasksScope& operator=(const ExplicitMicrotasksScope&) = delete;

 private:
  v8::MicrotaskQueue* microtask_queue_;
  v8::MicrotasksPolicy original_policy_;
};

}  // namespace electron::util

namespace electron::Buffer {

// Convenience function to view a Node buffer's data as a base::span().
// Analogous to base::as_byte_span()
[[nodiscard]] base::span<uint8_t> as_byte_span(
    v8::Local<v8::Value> node_buffer);

// span-friendly version of node::Buffer::Copy()
[[nodiscard]] v8::MaybeLocal<v8::Object> Copy(v8::Isolate* isolate,
                                              base::span<const char> data);

// span-friendly version of node::Buffer::Copy()
[[nodiscard]] v8::MaybeLocal<v8::Object> Copy(v8::Isolate* isolate,
                                              base::span<const uint8_t> data);

}  // namespace electron::Buffer

#endif  // ELECTRON_SHELL_COMMON_NODE_UTIL_H_

// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_UTIL_H_
#define ELECTRON_SHELL_COMMON_NODE_UTIL_H_

#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "v8/include/v8-forward.h"

namespace node {
class Environment;
}

namespace electron::util {

// Emit a warning via node's process.emitWarning()
void EmitWarning(v8::Isolate* isolate,
                 std::string_view warning_msg,
                 std::string_view warning_type);

// Emit a warning via node's process.emitWarning(),
// using JavscriptEnvironment's isolate
void EmitWarning(std::string_view warning_msg, std::string_view warning_type);

// Run a script with JS source bundled inside the binary as if it's wrapped
// in a function called with a null receiver and arguments specified in C++.
// The returned value is empty if an exception is encountered.
// JS code run with this method can assume that their top-level
// declarations won't affect the global scope.
v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments);

// Convenience function to view a Node buffer's data as a base::span().
// Analogous to base::as_byte_span()
[[nodiscard]] base::span<uint8_t> as_byte_span(
    v8::Local<v8::Value> node_buffer);

}  // namespace electron::util

#endif  // ELECTRON_SHELL_COMMON_NODE_UTIL_H_

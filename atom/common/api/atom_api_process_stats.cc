// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_native_image.h"

#include <string>
#include <vector>

#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"

namespace {

int32_t AddTwoNumbers(int32_t a, int32_t b) {
  return a + b;
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("addTwoNumbers", &AddTwoNumbers);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_process_stats, Initialize)

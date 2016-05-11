// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_key_weak_map.h"

#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createIDWeakMap",
                 &atom::api::KeyWeakMap<int32_t>::Create);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_id_weak_map, Initialize)

// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_emitter.h"
#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"

namespace {

v8::Local<v8::Object> CreateWithSender(v8::Isolate* isolate,
                                       v8::Local<v8::Object> sender) {
  return mate::internal::CreateJSEvent(isolate, sender, nullptr, nullptr);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createWithSender", &CreateWithSender);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_event, Initialize)

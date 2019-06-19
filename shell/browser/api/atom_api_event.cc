// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "native_mate/dictionary.h"
#include "shell/browser/api/event_emitter.h"
#include "shell/common/node_includes.h"

namespace {

v8::Local<v8::Object> CreateWithSender(v8::Isolate* isolate,
                                       v8::Local<v8::Object> sender) {
  return mate::internal::CreateJSEvent(isolate, sender, nullptr, base::nullopt);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createWithSender", &CreateWithSender);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_event, Initialize)

// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/node_event_emitter.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8-context.h"

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict{v8::Isolate::GetCurrent(), exports};
  dict.Set("EventEmitter", gin_helper::GetNodeEventEmitterConstructor(context));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_events, Initialize)

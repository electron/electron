// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/no_destructor.h"
#include "gin/per_isolate_data.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_mixin.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod(
      "setEventEmitterPrototype",
      base::BindRepeating(&gin_helper::internal::SetEventEmitterPrototype));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_event_emitter, Initialize)

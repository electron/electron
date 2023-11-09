// Copyright (c) 2022 Asana, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_push_notifications.h"

#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron::api {

PushNotifications* g_push_notifications = nullptr;

gin::WrapperInfo PushNotifications::kWrapperInfo = {gin::kEmbedderNativeGin};

PushNotifications::PushNotifications() = default;

PushNotifications::~PushNotifications() {
  g_push_notifications = nullptr;
}

// static
PushNotifications* PushNotifications::Get() {
  if (!g_push_notifications)
    g_push_notifications = new PushNotifications();
  return g_push_notifications;
}

// static
gin::Handle<PushNotifications> PushNotifications::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, PushNotifications::Get());
}

// static
gin::ObjectTemplateBuilder PushNotifications::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  auto builder = gin_helper::EventEmitterMixin<
      PushNotifications>::GetObjectTemplateBuilder(isolate);
#if BUILDFLAG(IS_MAC)
  builder
      .SetMethod("registerForAPNSNotifications",
                 &PushNotifications::RegisterForAPNSNotifications)
      .SetMethod("unregisterForAPNSNotifications",
                 &PushNotifications::UnregisterForAPNSNotifications);
#endif
  return builder;
}

const char* PushNotifications::GetTypeName() {
  return "PushNotifications";
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("pushNotifications",
           electron::api::PushNotifications::Create(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_push_notifications,
                                  Initialize)

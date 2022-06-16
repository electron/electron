// Copyright (c) 2022 Asana, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_push_notifications.h"
#include "shell/app/electron_main_delegate.h"

#include <string>

#import "shell/browser/mac/electron_application.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

PushNotifications* push_notifications = nullptr;

gin::WrapperInfo PushNotifications::kWrapperInfo = {gin::kEmbedderNativeGin};

PushNotifications::PushNotifications() {
  DCHECK(!push_notifications);
  push_notifications = this;
}

PushNotifications::~PushNotifications() {
  DCHECK(push_notifications);
  push_notifications = nullptr;
}

#if BUILDFLAG(IS_MAC)

void PushNotifications::RegisterForAPNSNotifications() {
  [[AtomApplication sharedApplication]
      registerForRemoteNotificationTypes:NSRemoteNotificationTypeBadge |
                                         NSRemoteNotificationTypeAlert |
                                         NSRemoteNotificationTypeSound];
}

void PushNotifications::UnregisterForAPNSNotifications() {
  [[AtomApplication sharedApplication] unregisterForRemoteNotifications];
}

void PushNotifications::OnDidRegisterForAPNSNotificationsWithDeviceToken(
    const std::string& token) {
  Emit("registered-for-apns-notifications", token);
}

void PushNotifications::OnDidFailToRegisterForAPNSNotificationsWithError(
    const std::string& error) {
  Emit("failed-to-register-for-apns-notifications", error);
}

void PushNotifications::OnDidReceiveAPNSNotification(
    const base::DictionaryValue& user_info) {
  Emit("received-apns-notification", user_info);
}
#endif

// static
PushNotifications* PushNotifications::Get() {
  return push_notifications;
}

// static
gin::Handle<PushNotifications> PushNotifications::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new PushNotifications());
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

}  // namespace api

}  // namespace electron

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

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_push_notifications,
                                 Initialize)

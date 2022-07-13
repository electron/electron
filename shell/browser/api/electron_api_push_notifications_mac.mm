// Copyright (c) 2022 Asana, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_push_notifications.h"

#include <string>

#include <utility>
#include <vector>
#import "shell/browser/mac/electron_application.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/promise.h"

namespace electron {

namespace api {

v8::Local<v8::Promise> PushNotifications::RegisterForAPNSNotifications(
    v8::Isolate* isolate) {
  gin_helper::Promise<std::string> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  [[AtomApplication sharedApplication]
      registerForRemoteNotificationTypes:NSRemoteNotificationTypeBadge |
                                         NSRemoteNotificationTypeAlert |
                                         NSRemoteNotificationTypeSound];

  PushNotifications::apns_promise_set_.emplace_back(std::move(promise));
  return handle;
}

void PushNotifications::ResolveAPNSPromiseSetWithToken(
    const std::string& token_string) {
  std::vector<gin_helper::Promise<std::string>> promises =
      std::move(PushNotifications::apns_promise_set_);
  for (auto& promise : promises) {
    promise.Resolve(token_string);
  }
}

void PushNotifications::RejectAPNSPromiseSetWithError(
    const std::string& error_message) {
  std::vector<gin_helper::Promise<std::string>> promises =
      std::move(PushNotifications::apns_promise_set_);
  for (auto& promise : promises) {
    promise.RejectWithErrorMessage(error_message);
  }
}

void PushNotifications::UnregisterForAPNSNotifications() {
  [[AtomApplication sharedApplication] unregisterForRemoteNotifications];
}

void PushNotifications::OnDidReceiveAPNSNotification(
    const base::Value::Dict& user_info) {
  Emit("received-apns-notification", user_info);
}

}  // namespace api

}  // namespace electron

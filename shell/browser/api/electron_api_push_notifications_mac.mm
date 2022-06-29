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

// This set maintains all the promises that should be fulfilled
// once macOS registers, or fails to register, for APNS
std::vector<gin_helper::Promise<std::string>> apns_promise_set_;

v8::Local<v8::Promise> PushNotifications::RegisterForAPNSNotifications(
    v8::Isolate* isolate) {
  gin_helper::Promise<std::string> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  [[AtomApplication sharedApplication]
      registerForRemoteNotificationTypes:NSRemoteNotificationTypeBadge |
                                         NSRemoteNotificationTypeAlert |
                                         NSRemoteNotificationTypeSound];

  apns_promise_set_.emplace_back(std::move(promise));
  return handle;
}

void PushNotifications::ResolveAPNSPromiseSetWithToken(
    const std::string& token_string) {
  for (auto& promise : apns_promise_set_) {
    promise.Resolve(token_string);
  }
  apns_promise_set_.clear();
}

void PushNotifications::ResolveAPNSPromiseSetWithError(
    const std::string& error_message) {
  for (auto& promise : apns_promise_set_) {
    promise.RejectWithErrorMessage(error_message);
  }
  apns_promise_set_.clear();
}

void PushNotifications::UnregisterForAPNSNotifications() {
  [[AtomApplication sharedApplication] unregisterForRemoteNotifications];
}

void PushNotifications::OnDidReceiveAPNSNotification(
    const base::DictionaryValue& user_info) {
  Emit("received-apns-notification", user_info);
}

}  // namespace api

}  // namespace electron

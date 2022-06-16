// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_

#include <string>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/event_emitter_mixin.h"

namespace electron {

namespace api {

class PushNotifications
    : public ElectronBrowserClient::Delegate,
      public gin::Wrappable<PushNotifications>,
      public gin_helper::EventEmitterMixin<PushNotifications>,
      public BrowserObserver {
 public:
  static PushNotifications* Get();
  static gin::Handle<PushNotifications> Create(v8::Isolate* isolate);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  PushNotifications(const PushNotifications&) = delete;
  PushNotifications& operator=(const PushNotifications&) = delete;

  void OnDidRegisterForAPNSNotificationsWithDeviceToken(
      const std::string& token);
  void OnDidFailToRegisterForAPNSNotificationsWithError(
      const std::string& error);
  void OnDidReceiveAPNSNotification(const base::DictionaryValue& user_info);

 private:
  PushNotifications();
  ~PushNotifications() override;

  void RegisterForAPNSNotifications();
  void UnregisterForAPNSNotifications();
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_

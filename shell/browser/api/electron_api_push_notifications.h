// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/electron_browser_client.h"

namespace electron {

namespace api {

class PushNotifications
    : public ElectronBrowserClient::Delegate,
      public gin::Wrappable<PushNotifications>,
      public gin_helper::EventEmitterMixin<PushNotifications>,
      public BrowserObserver {
 public:
  static gin::Handle<PushNotifications> Create(v8::Isolate* isolate);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  PushNotifications(const PushNotifications&) = delete;
  PushNotifications& operator=(const PushNotifications&) = delete;

 private:
  explicit PushNotifications();
  ~PushNotifications() override;

  // BrowserObserver
#if BUILDFLAG(IS_MAC)
  void OnDidRegisterForAPNSNotificationsWithDeviceToken(
    const std::string& token) override;
  void OnDidFailToRegisterForAPNSNotificationsWithError(
      const std::string& error) override;
  void OnDidReceiveAPNSNotification(
      const base::DictionaryValue& user_info) override;
#endif
};

}  // namespace api

}  // namespace electron

#endif // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_

// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "gin/per_isolate_data.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/promise.h"

namespace electron::api {

class PushNotifications final
    : public gin::Wrappable<PushNotifications>,
      public gin_helper::EventEmitterMixin<PushNotifications>,
      public gin::PerIsolateData::DisposeObserver {
 public:
  static PushNotifications* Get();

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "PushNotifications"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override {}
  void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) override;
  void OnDisposed() override {}

  // disable copy
  PushNotifications(const PushNotifications&) = delete;
  PushNotifications& operator=(const PushNotifications&) = delete;

#if BUILDFLAG(IS_MAC)
  void OnDidReceiveAPNSNotification(const base::DictValue& user_info);
  void ResolveAPNSPromiseSetWithToken(const std::string& token_string);
  void RejectAPNSPromiseSetWithError(const std::string& error_message);
#endif

  // Public for cppgc::MakeGarbageCollected.
  explicit PushNotifications(v8::Isolate* isolate);
  ~PushNotifications() override;

 private:
  // This set maintains all the promises that should be fulfilled
  // once macOS registers, or fails to register, for APNS
  std::vector<gin_helper::Promise<std::string>> apns_promise_set_;

#if BUILDFLAG(IS_MAC)
  v8::Local<v8::Promise> RegisterForAPNSNotifications(v8::Isolate* isolate);
  void UnregisterForAPNSNotifications();
#endif
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_PUSH_NOTIFICATIONS_H_

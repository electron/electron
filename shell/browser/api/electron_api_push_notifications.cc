// Copyright (c) 2022 Asana, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_push_notifications.h"

#include "base/no_destructor.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

gin::WrapperInfo PushNotifications::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronPushNotifications);

PushNotifications::PushNotifications(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->AddDisposeObserver(this);
}

PushNotifications::~PushNotifications() = default;

void PushNotifications::OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) {
  gin::PerIsolateData::From(isolate)->RemoveDisposeObserver(this);
  apns_promise_set_.clear();
}

// static
PushNotifications* PushNotifications::Get() {
  static base::NoDestructor<cppgc::Persistent<PushNotifications>> instance;
  if (!*instance) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    *instance = cppgc::MakeGarbageCollected<PushNotifications>(
        isolate->GetCppHeap()->GetAllocationHandle(), isolate);
  }
  return instance->Get();
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

const gin::WrapperInfo* PushNotifications::wrapper_info() const {
  return &kWrapperInfo;
}

const char* PushNotifications::GetHumanReadableName() const {
  return "Electron / PushNotifications";
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("pushNotifications", electron::api::PushNotifications::Get());
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_push_notifications,
                                  Initialize)

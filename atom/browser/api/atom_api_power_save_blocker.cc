// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_save_blocker.h"

#include "content/public/browser/power_save_blocker.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<content::PowerSaveBlocker::PowerSaveBlockerType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::PowerSaveBlocker::PowerSaveBlockerType* out) {
    using content::PowerSaveBlocker;
    int type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    switch (static_cast<PowerSaveBlocker::PowerSaveBlockerType>(type)) {
      case PowerSaveBlocker::kPowerSaveBlockPreventAppSuspension:
        *out = PowerSaveBlocker::kPowerSaveBlockPreventAppSuspension;
        break;
      case PowerSaveBlocker::kPowerSaveBlockPreventDisplaySleep:
        *out = PowerSaveBlocker::kPowerSaveBlockPreventDisplaySleep;
        break;
      default:
        return false;
    }
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

PowerSaveBlocker::PowerSaveBlocker() {
}

PowerSaveBlocker::~PowerSaveBlocker() {
}

void PowerSaveBlocker::Start(
    content::PowerSaveBlocker::PowerSaveBlockerType type) {
  power_save_blocker_ = content::PowerSaveBlocker::Create(
      type,
      content::PowerSaveBlocker::kReasonOther,
      "Users required");
}

void PowerSaveBlocker::Stop() {
  power_save_blocker_.reset();
}

bool PowerSaveBlocker::IsStarted() {
  return power_save_blocker_.get() != NULL;
}

mate::ObjectTemplateBuilder PowerSaveBlocker::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("start", &PowerSaveBlocker::Start)
      .SetMethod("stop", &PowerSaveBlocker::Stop);
      .SetMethod("isStarted", &PowerSaveBlocker::IsStarted)
}

// static
mate::Handle<PowerSaveBlocker> PowerSaveBlocker::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new PowerSaveBlocker);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("powerSaveBlocker", atom::api::PowerSaveBlocker::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_power_save_blocker, Initialize);

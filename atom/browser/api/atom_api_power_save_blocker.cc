// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_save_blocker.h"

#include <string>

#include "atom/common/node_includes.h"
#include "content/public/browser/power_save_blocker.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<content::PowerSaveBlocker::PowerSaveBlockerType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::PowerSaveBlocker::PowerSaveBlockerType* out) {
    using content::PowerSaveBlocker;
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "prevent-app-suspension")
      *out = PowerSaveBlocker::kPowerSaveBlockPreventAppSuspension;
    else if (type == "prevent-display-sleep")
      *out = PowerSaveBlocker::kPowerSaveBlockPreventDisplaySleep;
    else
      return false;
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

PowerSaveBlocker::PowerSaveBlocker(v8::Isolate* isolate)
    : current_blocker_type_(
          content::PowerSaveBlocker::kPowerSaveBlockPreventAppSuspension) {
  Init(isolate);
}

PowerSaveBlocker::~PowerSaveBlocker() {
}

void PowerSaveBlocker::UpdatePowerSaveBlocker() {
  if (power_save_blocker_types_.empty()) {
    power_save_blocker_.reset();
    return;
  }

  // |kPowerSaveBlockPreventAppSuspension| keeps system active, but allows
  // screen to be turned off.
  // |kPowerSaveBlockPreventDisplaySleep| keeps system and screen active, has a
  // higher precedence level than |kPowerSaveBlockPreventAppSuspension|.
  //
  // Only the highest-precedence blocker type takes effect.
  content::PowerSaveBlocker::PowerSaveBlockerType new_blocker_type =
      content::PowerSaveBlocker::kPowerSaveBlockPreventAppSuspension;
  for (const auto& element : power_save_blocker_types_) {
    if (element.second ==
        content::PowerSaveBlocker::kPowerSaveBlockPreventDisplaySleep) {
      new_blocker_type =
          content::PowerSaveBlocker::kPowerSaveBlockPreventDisplaySleep;
      break;
    }
  }

  if (!power_save_blocker_ || new_blocker_type != current_blocker_type_) {
    std::unique_ptr<content::PowerSaveBlocker> new_blocker =
        content::PowerSaveBlocker::Create(
            new_blocker_type,
            content::PowerSaveBlocker::kReasonOther,
            ATOM_PRODUCT_NAME);
    power_save_blocker_.swap(new_blocker);
    current_blocker_type_ = new_blocker_type;
  }
}

int PowerSaveBlocker::Start(
    content::PowerSaveBlocker::PowerSaveBlockerType type) {
  static int count = 0;
  power_save_blocker_types_[count] = type;
  UpdatePowerSaveBlocker();
  return count++;
}

bool PowerSaveBlocker::Stop(int id) {
  bool success = power_save_blocker_types_.erase(id) > 0;
  UpdatePowerSaveBlocker();
  return success;
}

bool PowerSaveBlocker::IsStarted(int id) {
  return power_save_blocker_types_.find(id) != power_save_blocker_types_.end();
}

// static
mate::Handle<PowerSaveBlocker> PowerSaveBlocker::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new PowerSaveBlocker(isolate));
}

// static
void PowerSaveBlocker::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "PowerSaveBlocker"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("start", &PowerSaveBlocker::Start)
      .SetMethod("stop", &PowerSaveBlocker::Stop)
      .SetMethod("isStarted", &PowerSaveBlocker::IsStarted);
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

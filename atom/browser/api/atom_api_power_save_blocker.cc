// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_save_blocker.h"

#include <string>

#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace mate {

template <>
struct Converter<device::mojom::WakeLockType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     device::mojom::WakeLockType* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "prevent-app-suspension")
      *out = device::mojom::WakeLockType::kPreventAppSuspension;
    else if (type == "prevent-display-sleep")
      *out = device::mojom::WakeLockType::kPreventDisplaySleep;
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
          device::mojom::WakeLockType::kPreventAppSuspension) {
  Init(isolate);
}

PowerSaveBlocker::~PowerSaveBlocker() {}

void PowerSaveBlocker::UpdatePowerSaveBlocker() {
  if (power_save_blocker_types_.empty()) {
    power_save_blocker_.reset();
    return;
  }

  // |WakeLockType::kPreventAppSuspension| keeps system active, but allows
  // screen to be turned off.
  // |WakeLockType::kPreventDisplaySleep| keeps system and screen active, has a
  // higher precedence level than |WakeLockType::kPreventAppSuspension|.
  //
  // Only the highest-precedence blocker type takes effect.
  device::mojom::WakeLockType new_blocker_type =
      device::mojom::WakeLockType::kPreventAppSuspension;
  for (const auto& element : power_save_blocker_types_) {
    if (element.second ==
        device::mojom::WakeLockType::kPreventDisplaySleep) {
      new_blocker_type =
          device::mojom::WakeLockType::kPreventDisplaySleep;
      break;
    }
  }

  if (!power_save_blocker_ || new_blocker_type != current_blocker_type_) {
    auto new_blocker = std::make_unique<device::PowerSaveBlocker>(
        new_blocker_type, device::mojom::WakeLockReason::kOther,
        ATOM_PRODUCT_NAME, base::ThreadTaskRunnerHandle::Get(),
        // This task runner may be used by some device service
        // implementation bits to interface with dbus client code, which in
        // turn imposes some subtle thread affinity on the clients. We
        // therefore require a single-thread runner.
        base::CreateSingleThreadTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::BACKGROUND}));
    power_save_blocker_.swap(new_blocker);
    current_blocker_type_ = new_blocker_type;
  }
}

int PowerSaveBlocker::Start(device::mojom::WakeLockType type) {
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
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "PowerSaveBlocker"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("start", &PowerSaveBlocker::Start)
      .SetMethod("stop", &PowerSaveBlocker::Stop)
      .SetMethod("isStarted", &PowerSaveBlocker::IsStarted);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("powerSaveBlocker", atom::api::PowerSaveBlocker::Create(isolate));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_power_save_blocker, Initialize);

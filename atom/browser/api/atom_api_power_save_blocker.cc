// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_save_blocker.h"

#include <string>

#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/service_manager_connection.h"
#include "native_mate/dictionary.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

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
    : current_lock_type_(device::mojom::WakeLockType::kPreventAppSuspension),
      is_wake_lock_active_(false) {
  Init(isolate);
}

PowerSaveBlocker::~PowerSaveBlocker() {}

void PowerSaveBlocker::UpdatePowerSaveBlocker() {
  if (wake_lock_types_.empty()) {
    if (is_wake_lock_active_) {
      GetWakeLock()->CancelWakeLock();
      is_wake_lock_active_ = false;
    }
    return;
  }

  // |WakeLockType::kPreventAppSuspension| keeps system active, but allows
  // screen to be turned off.
  // |WakeLockType::kPreventDisplaySleep| keeps system and screen active, has a
  // higher precedence level than |WakeLockType::kPreventAppSuspension|.
  //
  // Only the highest-precedence blocker type takes effect.
  device::mojom::WakeLockType new_lock_type =
      device::mojom::WakeLockType::kPreventAppSuspension;
  for (const auto& element : wake_lock_types_) {
    if (element.second == device::mojom::WakeLockType::kPreventDisplaySleep) {
      new_lock_type = device::mojom::WakeLockType::kPreventDisplaySleep;
      break;
    }
  }

  if (current_lock_type_ != new_lock_type) {
    GetWakeLock()->ChangeType(new_lock_type, base::DoNothing());
    current_lock_type_ = new_lock_type;
  }
  if (!is_wake_lock_active_) {
    GetWakeLock()->RequestWakeLock();
    is_wake_lock_active_ = true;
  }
}

device::mojom::WakeLock* PowerSaveBlocker::GetWakeLock() {
  if (!wake_lock_) {
    device::mojom::WakeLockProviderPtr wake_lock_provider;
    DCHECK(content::ServiceManagerConnection::GetForProcess());
    auto* connector =
        content::ServiceManagerConnection::GetForProcess()->GetConnector();
    connector->BindInterface(device::mojom::kServiceName,
                             mojo::MakeRequest(&wake_lock_provider));

    wake_lock_provider->GetWakeLockWithoutContext(
        device::mojom::WakeLockType::kPreventAppSuspension,
        device::mojom::WakeLockReason::kOther, ATOM_PRODUCT_NAME,
        mojo::MakeRequest(&wake_lock_));
  }
  return wake_lock_.get();
}

int PowerSaveBlocker::Start(device::mojom::WakeLockType type) {
  static int count = 0;
  wake_lock_types_[count] = type;
  UpdatePowerSaveBlocker();
  return count++;
}

bool PowerSaveBlocker::Stop(int id) {
  bool success = wake_lock_types_.erase(id) > 0;
  UpdatePowerSaveBlocker();
  return success;
}

bool PowerSaveBlocker::IsStarted(int id) {
  return wake_lock_types_.find(id) != wake_lock_types_.end();
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

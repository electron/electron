// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_power_save_blocker.h"

#include <string>

#include "base/functional/callback_helpers.h"
#include "content/public/browser/device_service.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_includes.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace gin {

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

}  // namespace gin

namespace electron::api {

const gin::WrapperInfo PowerSaveBlocker::kWrapperInfo = {
    {gin::kEmbedderNativeGin},
    gin::kElectronPowerSaveBlocker};

PowerSaveBlocker::PowerSaveBlocker(v8::Isolate* isolate)
    : current_lock_type_(device::mojom::WakeLockType::kPreventAppSuspension) {}

PowerSaveBlocker::~PowerSaveBlocker() = default;

// static
gin_helper::Handle<PowerSaveBlocker> PowerSaveBlocker::Create(
    v8::Isolate* isolate) {
  return gin_helper::CreateHandle(
      isolate, cppgc::MakeGarbageCollected<PowerSaveBlocker>(
                   isolate->GetCppHeap()->GetAllocationHandle(), isolate));
}

const gin::WrapperInfo* PowerSaveBlocker::wrapper_info() const {
  return &kWrapperInfo;
}

const char* PowerSaveBlocker::GetHumanReadableName() const {
  return "Electron / PowerSaveBlocker";
}

gin::ObjectTemplateBuilder PowerSaveBlocker::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<PowerSaveBlocker>::GetObjectTemplateBuilder(isolate)
      .SetMethod("start", &PowerSaveBlocker::Start)
      .SetMethod("stop", &PowerSaveBlocker::Stop)
      .SetMethod("isStarted", &PowerSaveBlocker::IsStarted);
}

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
    mojo::Remote<device::mojom::WakeLockProvider> wake_lock_provider;
    content::GetDeviceService().BindWakeLockProvider(
        wake_lock_provider.BindNewPipeAndPassReceiver());

    wake_lock_provider->GetWakeLockWithoutContext(
        device::mojom::WakeLockType::kPreventAppSuspension,
        device::mojom::WakeLockReason::kOther, ELECTRON_PRODUCT_NAME,
        wake_lock_.BindNewPipeAndPassReceiver());
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

bool PowerSaveBlocker::IsStarted(int id) const {
  return wake_lock_types_.contains(id);
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin::Dictionary dict{isolate, exports};
  dict.Set("powerSaveBlocker",
           electron::api::PowerSaveBlocker::Create(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_power_save_blocker,
                                  Initialize)

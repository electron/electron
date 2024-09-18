// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_power_monitor.h"

#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "base/power_monitor/power_observer.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<ui::IdleState> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::IdleState& in) {
    switch (in) {
      case ui::IDLE_STATE_ACTIVE:
        return StringToV8(isolate, "active");
      case ui::IDLE_STATE_IDLE:
        return StringToV8(isolate, "idle");
      case ui::IDLE_STATE_LOCKED:
        return StringToV8(isolate, "locked");
      case ui::IDLE_STATE_UNKNOWN:
      default:
        return StringToV8(isolate, "unknown");
    }
  }
};

template <>
struct Converter<base::PowerThermalObserver::DeviceThermalState> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const base::PowerThermalObserver::DeviceThermalState& in) {
    switch (in) {
      case base::PowerThermalObserver::DeviceThermalState::kUnknown:
        return StringToV8(isolate, "unknown");
      case base::PowerThermalObserver::DeviceThermalState::kNominal:
        return StringToV8(isolate, "nominal");
      case base::PowerThermalObserver::DeviceThermalState::kFair:
        return StringToV8(isolate, "fair");
      case base::PowerThermalObserver::DeviceThermalState::kSerious:
        return StringToV8(isolate, "serious");
      case base::PowerThermalObserver::DeviceThermalState::kCritical:
        return StringToV8(isolate, "critical");
    }
  }
};

}  // namespace gin

namespace electron::api {

gin::WrapperInfo PowerMonitor::kWrapperInfo = {gin::kEmbedderNativeGin};

PowerMonitor::PowerMonitor(v8::Isolate* isolate) {
#if BUILDFLAG(IS_MAC)
  Browser::Get()->SetShutdownHandler(base::BindRepeating(
      &PowerMonitor::ShouldShutdown, base::Unretained(this)));
#endif

  auto* power_monitor = base::PowerMonitor::GetInstance();
  power_monitor->AddPowerStateObserver(this);
  power_monitor->AddPowerSuspendObserver(this);
  power_monitor->AddPowerThermalObserver(this);

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  InitPlatformSpecificMonitors();
#endif
}

PowerMonitor::~PowerMonitor() {
  auto* power_monitor = base::PowerMonitor::GetInstance();
  power_monitor->RemovePowerStateObserver(this);
  power_monitor->RemovePowerSuspendObserver(this);
  power_monitor->RemovePowerThermalObserver(this);
}

bool PowerMonitor::ShouldShutdown() {
  return !Emit("shutdown");
}

void PowerMonitor::OnBatteryPowerStatusChange(
    BatteryPowerStatus battery_power_status) {
  switch (battery_power_status) {
    case BatteryPowerStatus::kBatteryPower:
      Emit("on-battery");
      break;
    case BatteryPowerStatus::kExternalPower:
      Emit("on-ac");
      break;
    case BatteryPowerStatus::kUnknown:
      // Ignored
      break;
  }
}

void PowerMonitor::OnSuspend() {
  Emit("suspend");
}

void PowerMonitor::OnResume() {
  Emit("resume");
}

void PowerMonitor::OnThermalStateChange(DeviceThermalState new_state) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent(
      "thermal-state-change",
      gin::DataObjectBuilder(isolate).Set("state", new_state).Build());
}

void PowerMonitor::OnSpeedLimitChange(int speed_limit) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  EmitWithoutEvent(
      "speed-limit-change",
      gin::DataObjectBuilder(isolate).Set("limit", speed_limit).Build());
}

#if BUILDFLAG(IS_LINUX)
void PowerMonitor::SetListeningForShutdown(bool is_listening) {
  if (is_listening) {
    // unretained is OK because we own power_observer_linux_
    power_observer_linux_.SetShutdownHandler(base::BindRepeating(
        &PowerMonitor::ShouldShutdown, base::Unretained(this)));
  } else {
    power_observer_linux_.SetShutdownHandler(base::RepeatingCallback<bool()>());
  }
}
#endif

// static
v8::Local<v8::Value> PowerMonitor::Create(v8::Isolate* isolate) {
  auto* pm = new PowerMonitor(isolate);
  auto handle = gin::CreateHandle(isolate, pm).ToV8();
  pm->Pin(isolate);
  return handle;
}

gin::ObjectTemplateBuilder PowerMonitor::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  auto builder =
      gin_helper::EventEmitterMixin<PowerMonitor>::GetObjectTemplateBuilder(
          isolate);
#if BUILDFLAG(IS_LINUX)
  builder.SetMethod("setListeningForShutdown",
                    &PowerMonitor::SetListeningForShutdown);
#endif
  return builder;
}

const char* PowerMonitor::GetTypeName() {
  return "PowerMonitor";
}

}  // namespace electron::api

namespace {

using electron::api::PowerMonitor;

ui::IdleState GetSystemIdleState(v8::Isolate* isolate, int idle_threshold) {
  if (idle_threshold > 0) {
    return ui::CalculateIdleState(idle_threshold);
  } else {
    isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
        isolate, "Invalid idle threshold, must be greater than 0")));
    return ui::IDLE_STATE_UNKNOWN;
  }
}

int GetSystemIdleTime() {
  return ui::CalculateIdleTime();
}

bool IsOnBatteryPower() {
  return base::PowerMonitor::GetInstance()->IsOnBatteryPower();
}

base::PowerThermalObserver::DeviceThermalState GetCurrentThermalState() {
  return base::PowerMonitor::GetInstance()->GetCurrentThermalState();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("createPowerMonitor",
                 base::BindRepeating(&PowerMonitor::Create));
  dict.SetMethod("getSystemIdleState",
                 base::BindRepeating(&GetSystemIdleState));
  dict.SetMethod("getCurrentThermalState",
                 base::BindRepeating(&GetCurrentThermalState));
  dict.SetMethod("getSystemIdleTime", base::BindRepeating(&GetSystemIdleTime));
  dict.SetMethod("isOnBatteryPower", base::BindRepeating(&IsOnBatteryPower));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_power_monitor, Initialize)

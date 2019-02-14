// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_monitor.h"

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/callback.h"
#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace mate {
template <>
struct Converter<ui::IdleState> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::IdleState& in) {
    switch (in) {
      case ui::IDLE_STATE_ACTIVE:
        return mate::StringToV8(isolate, "active");
      case ui::IDLE_STATE_IDLE:
        return mate::StringToV8(isolate, "idle");
      case ui::IDLE_STATE_LOCKED:
        return mate::StringToV8(isolate, "locked");
      case ui::IDLE_STATE_UNKNOWN:
      default:
        return mate::StringToV8(isolate, "unknown");
    }
  }
};
}  // namespace mate

namespace atom {

namespace api {

PowerMonitor::PowerMonitor(v8::Isolate* isolate) {
#if defined(OS_LINUX)
  SetShutdownHandler(
      base::Bind(&PowerMonitor::ShouldShutdown, base::Unretained(this)));
#elif defined(OS_MACOSX)
  Browser::Get()->SetShutdownHandler(
      base::Bind(&PowerMonitor::ShouldShutdown, base::Unretained(this)));
#endif
  base::PowerMonitor::Get()->AddObserver(this);
  Init(isolate);
#if defined(OS_MACOSX) || defined(OS_WIN)
  InitPlatformSpecificMonitors();
#endif
}

PowerMonitor::~PowerMonitor() {
  base::PowerMonitor::Get()->RemoveObserver(this);
}

bool PowerMonitor::ShouldShutdown() {
  return !Emit("shutdown");
}

#if defined(OS_LINUX)
void PowerMonitor::BlockShutdown() {
  PowerObserverLinux::BlockShutdown();
}

void PowerMonitor::UnblockShutdown() {
  PowerObserverLinux::UnblockShutdown();
}
#endif

void PowerMonitor::OnPowerStateChange(bool on_battery_power) {
  if (on_battery_power)
    Emit("on-battery");
  else
    Emit("on-ac");
}

void PowerMonitor::OnSuspend() {
  Emit("suspend");
}

void PowerMonitor::OnResume() {
  Emit("resume");
}

ui::IdleState PowerMonitor::QuerySystemIdleState(v8::Isolate* isolate,
                                                 int idle_threshold) {
  if (idle_threshold > 0) {
    return ui::CalculateIdleState(idle_threshold);
  } else {
    isolate->ThrowException(v8::Exception::TypeError(mate::StringToV8(
        isolate, "Invalid idle threshold, must be greater than 0")));
    return ui::IDLE_STATE_UNKNOWN;
  }
}

int PowerMonitor::QuerySystemIdleTime() {
  return ui::CalculateIdleTime();
}

// static
v8::Local<v8::Value> PowerMonitor::Create(v8::Isolate* isolate) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate,
        "Cannot require \"powerMonitor\" module before app is ready")));
    return v8::Null(isolate);
  }

  return mate::CreateHandle(isolate, new PowerMonitor(isolate)).ToV8();
}

// static
void PowerMonitor::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "PowerMonitor"));

  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
#if defined(OS_LINUX)
      .SetMethod("blockShutdown", &PowerMonitor::BlockShutdown)
      .SetMethod("unblockShutdown", &PowerMonitor::UnblockShutdown)
#endif
      .SetMethod("_querySystemIdleState", &PowerMonitor::QuerySystemIdleState)
      .SetMethod("_querySystemIdleTime", &PowerMonitor::QuerySystemIdleTime);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::PowerMonitor;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("powerMonitor", PowerMonitor::Create(isolate));
  dict.Set("PowerMonitor", PowerMonitor::GetConstructor(isolate)
                               ->GetFunction(context)
                               .ToLocalChecked());
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_power_monitor, Initialize)

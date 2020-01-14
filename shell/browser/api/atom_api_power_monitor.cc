// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_power_monitor.h"

#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/callback_converter.h"
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

}  // namespace gin

namespace electron {

namespace api {

PowerMonitor::PowerMonitor(v8::Isolate* isolate) {
#if defined(OS_LINUX)
  SetShutdownHandler(base::BindRepeating(&PowerMonitor::ShouldShutdown,
                                         base::Unretained(this)));
#elif defined(OS_MACOSX)
  Browser::Get()->SetShutdownHandler(base::BindRepeating(
      &PowerMonitor::ShouldShutdown, base::Unretained(this)));
#endif
  base::PowerMonitor::AddObserver(this);
  Init(isolate);
#if defined(OS_MACOSX) || defined(OS_WIN)
  InitPlatformSpecificMonitors();
#endif
}

PowerMonitor::~PowerMonitor() {
  base::PowerMonitor::RemoveObserver(this);
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

ui::IdleState PowerMonitor::GetSystemIdleState(v8::Isolate* isolate,
                                               int idle_threshold) {
  if (idle_threshold > 0) {
    return ui::CalculateIdleState(idle_threshold);
  } else {
    isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
        isolate, "Invalid idle threshold, must be greater than 0")));
    return ui::IDLE_STATE_UNKNOWN;
  }
}

int PowerMonitor::GetSystemIdleTime() {
  return ui::CalculateIdleTime();
}

// static
v8::Local<v8::Value> PowerMonitor::Create(v8::Isolate* isolate) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(
        gin::StringToV8(isolate,
                        "The 'powerMonitor' module can't be used before the "
                        "app 'ready' event")));
    return v8::Null(isolate);
  }

  return gin::CreateHandle(isolate, new PowerMonitor(isolate)).ToV8();
}

// static
void PowerMonitor::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "PowerMonitor"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
#if defined(OS_LINUX)
      .SetMethod("blockShutdown", &PowerMonitor::BlockShutdown)
      .SetMethod("unblockShutdown", &PowerMonitor::UnblockShutdown)
#endif
      .SetMethod("getSystemIdleState", &PowerMonitor::GetSystemIdleState)
      .SetMethod("getSystemIdleTime", &PowerMonitor::GetSystemIdleTime);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::PowerMonitor;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("createPowerMonitor", base::BindRepeating(&PowerMonitor::Create));
  dict.Set("PowerMonitor", PowerMonitor::GetConstructor(isolate)
                               ->GetFunction(context)
                               .ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_power_monitor, Initialize)

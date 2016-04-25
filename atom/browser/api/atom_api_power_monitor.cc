// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_monitor.h"

#include "atom/browser/browser.h"
#include "atom/common/node_includes.h"
#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

PowerMonitor::PowerMonitor(v8::Isolate* isolate) {
  base::PowerMonitor::Get()->AddObserver(this);
  Init(isolate);
}

PowerMonitor::~PowerMonitor() {
  base::PowerMonitor::Get()->RemoveObserver(this);
}

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

// static
v8::Local<v8::Value> PowerMonitor::Create(v8::Isolate* isolate) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate,
        "Cannot initialize \"power-monitor\" module before app is ready")));
    return v8::Null(isolate);
  }

  return mate::CreateHandle(isolate, new PowerMonitor(isolate)).ToV8();
}

// static
void PowerMonitor::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
#if defined(OS_MACOSX)
  base::PowerMonitorDeviceSource::AllocateSystemIOPorts();
#endif

  using atom::api::PowerMonitor;
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("powerMonitor", PowerMonitor::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_power_monitor, Initialize)

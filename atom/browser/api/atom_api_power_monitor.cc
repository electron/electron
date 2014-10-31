// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_power_monitor.h"

#include "base/power_monitor/power_monitor.h"
#include "base/power_monitor/power_monitor_device_source.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

PowerMonitor::PowerMonitor() {
  base::PowerMonitor::Get()->AddObserver(this);
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
mate::Handle<PowerMonitor> PowerMonitor::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new PowerMonitor);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
#if defined(OS_MACOSX)
  base::PowerMonitorDeviceSource::AllocateSystemIOPorts();
#endif

  using atom::api::PowerMonitor;
  v8::Isolate* isolate = context->GetIsolate();
  mate::Handle<PowerMonitor> power_monitor = PowerMonitor::Create(isolate);
  mate::Dictionary dict(isolate, exports);
  dict.Set("powerMonitor", power_monitor);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_power_monitor, Initialize)

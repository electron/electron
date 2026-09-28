// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_POWER_MONITOR_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_POWER_MONITOR_H_

#include "base/power_monitor/power_observer.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"

#if BUILDFLAG(IS_LINUX)
#include "shell/browser/lib/power_observer_linux.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <memory>

#include "base/win/windows_types.h"
#endif

namespace gin {
class ObjectTemplateBuilder;
}  // namespace gin

#if BUILDFLAG(IS_WIN)
namespace ui {
class SessionChangeObserver;
}  // namespace ui
#endif

namespace electron::api {

class PowerMonitor final : public gin::Wrappable<PowerMonitor>,
                           public gin_helper::EventEmitterMixin<PowerMonitor>,
                           private base::PowerStateObserver,
                           private base::PowerSuspendObserver,
                           private base::PowerThermalObserver {
 public:
  static PowerMonitor* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetClassName() const { return "PowerMonitor"; }

  // Make public for cppgc::MakeGarbageCollected.
  PowerMonitor();
  ~PowerMonitor() override;

  // disable copy
  PowerMonitor(const PowerMonitor&) = delete;
  PowerMonitor& operator=(const PowerMonitor&) = delete;

 private:
#if BUILDFLAG(IS_LINUX)
  void SetListeningForShutdown(bool);
#endif

  // Called by native callers.
  bool ShouldShutdown();

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  void InitPlatformSpecificMonitors();
  void DestroyPlatformSpecificMonitors();
#endif

  // base::PowerStateObserver implementations:
  void OnBatteryPowerStatusChange(
      BatteryPowerStatus battery_power_status) override;

  // base::PowerSuspendObserver implementations:
  void OnSuspend() override;
  void OnResume() override;

  // base::PowerThermalObserver
  void OnThermalStateChange(DeviceThermalState new_state) override;
  void OnSpeedLimitChange(int speed_limit) override;

#if BUILDFLAG(IS_WIN)
  void OnSessionChange(WPARAM wparam, const bool* is_current_session);

  std::unique_ptr<ui::SessionChangeObserver> session_change_observer_;
#endif

#if BUILDFLAG(IS_LINUX)
  PowerObserverLinux power_observer_linux_{this};
#endif
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_POWER_MONITOR_H_

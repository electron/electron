// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_POWER_MONITOR_H_
#define ATOM_BROWSER_API_ATOM_API_POWER_MONITOR_H_

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/lib/power_observer.h"
#include "base/compiler_specific.h"
#include "native_mate/handle.h"
#include "ui/base/idle/idle.h"

namespace atom {

namespace api {

class PowerMonitor : public mate::TrackableObject<PowerMonitor>,
                     public PowerObserver {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit PowerMonitor(v8::Isolate* isolate);
  ~PowerMonitor() override;

  // Called by native calles.
  bool ShouldShutdown();

#if defined(OS_LINUX)
  // Private JS APIs.
  void BlockShutdown();
  void UnblockShutdown();
#endif

#if defined(OS_MACOSX) || defined(OS_WIN)
  void InitPlatformSpecificMonitors();
#endif

  // base::PowerObserver implementations:
  void OnPowerStateChange(bool on_battery_power) override;
  void OnSuspend() override;
  void OnResume() override;

 private:
  void QuerySystemIdleState(v8::Isolate* isolate,
                            int idle_threshold,
                            const ui::IdleCallback& callback);
  void QuerySystemIdleTime(const ui::IdleTimeCallback& callback);

#if defined(OS_WIN)
  // Static callback invoked when a message comes in to our messaging window.
  static LRESULT CALLBACK WndProcStatic(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam);

  LRESULT CALLBACK WndProc(HWND hwnd,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam);

  // The window class of |window_|.
  ATOM atom_;

  // The handle of the module that contains the window procedure of |window_|.
  HMODULE instance_;

  // The window used for processing events.
  HWND window_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PowerMonitor);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_POWER_MONITOR_H_

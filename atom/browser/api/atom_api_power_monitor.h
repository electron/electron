// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_POWER_MONITOR_H_
#define ATOM_BROWSER_API_ATOM_API_POWER_MONITOR_H_

#include "base/compiler_specific.h"
#include "base/power_monitor/power_observer.h"
#include "atom/common/api/atom_api_event_emitter.h"

namespace atom {

namespace api {

class PowerMonitor : public EventEmitter,
                     public base::PowerObserver {
 public:
  virtual ~PowerMonitor();

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit PowerMonitor(v8::Handle<v8::Object> wrapper);

  virtual void OnPowerStateChange(bool on_battery_power) OVERRIDE;
  virtual void OnSuspend() OVERRIDE;
  virtual void OnResume() OVERRIDE;

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_COPY_AND_ASSIGN(PowerMonitor);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_POWER_MONITOR_H_

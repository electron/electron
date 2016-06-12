// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "atom/browser/api/trackable_object.h"
#include "base/compiler_specific.h"
#include "base/power_monitor/power_observer.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class PowerMonitor : public mate::TrackableObject<PowerMonitor>,
                     public base::PowerObserver {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  explicit PowerMonitor(v8::Isolate* isolate);
  ~PowerMonitor() override;

  // base::PowerObserver implementations:
  void OnPowerStateChange(bool on_battery_power) override;
  void OnSuspend() override;
  void OnResume() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerMonitor);
};

}  // namespace api

}  // namespace atom

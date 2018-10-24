// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_POWER_SAVE_BLOCKER_H_
#define ATOM_BROWSER_API_ATOM_API_POWER_SAVE_BLOCKER_H_

#include <map>
#include <memory>

#include "atom/browser/api/trackable_object.h"
#include "native_mate/handle.h"
#include "services/device/public/mojom/wake_lock.mojom.h"

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class PowerSaveBlocker : public mate::TrackableObject<PowerSaveBlocker> {
 public:
  static mate::Handle<PowerSaveBlocker> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit PowerSaveBlocker(v8::Isolate* isolate);
  ~PowerSaveBlocker() override;

 private:
  void UpdatePowerSaveBlocker();
  int Start(device::mojom::WakeLockType type);
  bool Stop(int id);
  bool IsStarted(int id);

  device::mojom::WakeLock* GetWakeLock();

  // Current wake lock level.
  device::mojom::WakeLockType current_lock_type_;

  // Whether the wake lock is currently active.
  bool is_wake_lock_active_;

  // Map from id to the corresponding blocker type for each request.
  using WakeLockTypeMap = std::map<int, device::mojom::WakeLockType>;
  WakeLockTypeMap wake_lock_types_;

  device::mojom::WakeLockPtr wake_lock_;

  DISALLOW_COPY_AND_ASSIGN(PowerSaveBlocker);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_POWER_SAVE_BLOCKER_H_

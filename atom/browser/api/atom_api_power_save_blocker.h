// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <map>

#include "atom/browser/api/trackable_object.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/power_save_blocker.h"
#include "native_mate/handle.h"

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class PowerSaveBlocker : public mate::TrackableObject<PowerSaveBlocker> {
 public:
  static mate::Handle<PowerSaveBlocker> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  explicit PowerSaveBlocker(v8::Isolate* isolate);
  ~PowerSaveBlocker() override;

 private:
  void UpdatePowerSaveBlocker();
  int Start(content::PowerSaveBlocker::PowerSaveBlockerType type);
  bool Stop(int id);
  bool IsStarted(int id);

  std::unique_ptr<content::PowerSaveBlocker> power_save_blocker_;

  // Currnet blocker type used by |power_save_blocker_|
  content::PowerSaveBlocker::PowerSaveBlockerType current_blocker_type_;

  // Map from id to the corresponding blocker type for each request.
  using PowerSaveBlockerTypeMap =
      std::map<int, content::PowerSaveBlocker::PowerSaveBlockerType>;
  PowerSaveBlockerTypeMap power_save_blocker_types_;

  DISALLOW_COPY_AND_ASSIGN(PowerSaveBlocker);
};

}  // namespace api

}  // namespace atom

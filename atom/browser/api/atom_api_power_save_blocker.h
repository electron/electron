// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_POWER_SAVE_BLOCKER_H_
#define ATOM_BROWSER_API_ATOM_API_POWER_SAVE_BLOCKER_H_

#include "base/memory/scoped_ptr.h"
#include "content/public/browser/power_save_blocker.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class PowerSaveBlocker : public mate::Wrappable {
 public:
  static mate::Handle<PowerSaveBlocker> Create(v8::Isolate* isolate);

 protected:
  PowerSaveBlocker();
  virtual ~PowerSaveBlocker();

  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  void Start(content::PowerSaveBlocker::PowerSaveBlockerType type);
  void Stop();
  bool IsStarted();

  scoped_ptr<content::PowerSaveBlocker> power_save_blocker_;
  DISALLOW_COPY_AND_ASSIGN(PowerSaveBlocker);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_POWER_SAVE_BLOCKER_H_

// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_
#define ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "base/callback.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class SystemPreferences : public mate::EventEmitter<SystemPreferences> {
 public:
  static mate::Handle<SystemPreferences> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

#if defined(OS_WIN)
  bool IsAeroGlassEnabled();
#elif defined(OS_MACOSX)
  int SubscribeNotification(const std::string& name,
                            const base::Closure& callback);
  void UnsubscribeNotification(int id);
#endif
  bool IsDarkMode();

 protected:
  explicit SystemPreferences(v8::Isolate* isolate);
  ~SystemPreferences() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemPreferences);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

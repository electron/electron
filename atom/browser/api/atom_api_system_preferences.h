// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_
#define ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "base/callback.h"
#include "native_mate/handle.h"

namespace base {
class DictionaryValue;
}

namespace atom {

namespace api {

class SystemPreferences : public mate::EventEmitter<SystemPreferences> {
 public:
  static mate::Handle<SystemPreferences> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if defined(OS_WIN)
  bool IsAeroGlassEnabled();
#elif defined(OS_MACOSX)
  using NotificationCallback = base::Callback<
    void(const std::string&, const base::DictionaryValue&)>;

  int SubscribeNotification(const std::string& name,
                            const NotificationCallback& callback);
  void UnsubscribeNotification(int id);
  int SubscribeLocalNotification(const std::string& name,
                                 const NotificationCallback& callback);
  void UnsubscribeLocalNotification(int request_id);
  v8::Local<v8::Value> GetUserDefault(const std::string& name,
                                      const std::string& type);
  bool IsSwipeTrackingFromScrollEventsEnabled();
#endif
  bool IsDarkMode();

 protected:
  explicit SystemPreferences(v8::Isolate* isolate);
  ~SystemPreferences() override;

#if defined(OS_MACOSX)
  int DoSubscribeNotification(const std::string& name,
                              const NotificationCallback& callback,
                              bool is_local);
  void DoUnsubscribeNotification(int request_id, bool is_local);
#endif

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemPreferences);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

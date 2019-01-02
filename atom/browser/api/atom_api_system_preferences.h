// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_
#define ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

#include <memory>
#include <string>

#include "atom/browser/api/event_emitter.h"
#include "atom/common/promise_util.h"
#include "base/callback.h"
#include "base/values.h"
#include "native_mate/handle.h"

#if defined(OS_WIN)
#include "atom/browser/browser.h"
#include "atom/browser/browser_observer.h"
#include "ui/gfx/sys_color_change_listener.h"
#endif

namespace base {
class DictionaryValue;
}

namespace atom {

namespace api {

#if defined(OS_MACOSX)
enum NotificationCenterKind {
  kNSDistributedNotificationCenter = 0,
  kNSNotificationCenter,
  kNSWorkspaceNotificationCenter,
};
#endif

class SystemPreferences : public mate::EventEmitter<SystemPreferences>
#if defined(OS_WIN)
    ,
                          public BrowserObserver,
                          public gfx::SysColorChangeListener
#endif
{
 public:
  static mate::Handle<SystemPreferences> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if defined(OS_WIN) || defined(OS_MACOSX)
  std::string GetAccentColor();
  std::string GetColor(const std::string& color, mate::Arguments* args);
#endif
#if defined(OS_WIN)
  bool IsAeroGlassEnabled();

  void InitializeWindow();

  // gfx::SysColorChangeListener:
  void OnSysColorChange() override;

  // BrowserObserver:
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;

#elif defined(OS_MACOSX)
  using NotificationCallback =
      base::Callback<void(const std::string&, const base::DictionaryValue&)>;

  void PostNotification(const std::string& name,
                        const base::DictionaryValue& user_info,
                        mate::Arguments* args);
  int SubscribeNotification(const std::string& name,
                            const NotificationCallback& callback);
  void UnsubscribeNotification(int id);
  void PostLocalNotification(const std::string& name,
                             const base::DictionaryValue& user_info);
  int SubscribeLocalNotification(const std::string& name,
                                 const NotificationCallback& callback);
  void UnsubscribeLocalNotification(int request_id);
  void PostWorkspaceNotification(const std::string& name,
                                 const base::DictionaryValue& user_info);
  int SubscribeWorkspaceNotification(const std::string& name,
                                     const NotificationCallback& callback);
  void UnsubscribeWorkspaceNotification(int request_id);
  v8::Local<v8::Value> GetUserDefault(const std::string& name,
                                      const std::string& type);
  void RegisterDefaults(mate::Arguments* args);
  void SetUserDefault(const std::string& name,
                      const std::string& type,
                      mate::Arguments* args);
  void RemoveUserDefault(const std::string& name);
  bool IsSwipeTrackingFromScrollEventsEnabled();

  std::string GetSystemColor(const std::string& color, mate::Arguments* args);

  static bool IsTrustedAccessibilityClient(bool prompt);

  // TODO(codebytere): Write tests for these methods once we
  // are running tests on a Mojave machine
  std::string GetMediaAccessStatus(const std::string& media_type,
                                   mate::Arguments* args);
  v8::Local<v8::Promise> AskForMediaAccess(v8::Isolate* isolate,
                                           const std::string& media_type);

  // TODO(MarshallOfSound): Write tests for these methods once we
  // are running tests on a Mojave machine
  v8::Local<v8::Value> GetEffectiveAppearance(v8::Isolate* isolate);
  v8::Local<v8::Value> GetAppLevelAppearance(v8::Isolate* isolate);
  void SetAppLevelAppearance(mate::Arguments* args);
#endif
  bool IsDarkMode();
  bool IsInvertedColorScheme();
  bool IsHighContrastColorScheme();

 protected:
  explicit SystemPreferences(v8::Isolate* isolate);
  ~SystemPreferences() override;

#if defined(OS_MACOSX)
  int DoSubscribeNotification(const std::string& name,
                              const NotificationCallback& callback,
                              NotificationCenterKind kind);
  void DoUnsubscribeNotification(int request_id, NotificationCenterKind kind);
#endif

 private:
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

  std::string current_color_;

  bool invertered_color_scheme_;

  bool high_contrast_color_scheme_;

  std::unique_ptr<gfx::ScopedSysColorChangeListener> color_change_listener_;
#endif
  DISALLOW_COPY_AND_ASSIGN(SystemPreferences);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

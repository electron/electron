// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"

#if BUILDFLAG(IS_WIN)
#include "shell/browser/browser.h"
#include "shell/browser/browser_observer.h"
#include "ui/gfx/sys_color_change_listener.h"
#endif

namespace electron::api {

#if BUILDFLAG(IS_MAC)
enum class NotificationCenterKind {
  kNSDistributedNotificationCenter = 0,
  kNSNotificationCenter,
  kNSWorkspaceNotificationCenter,
};
#endif

class SystemPreferences
    : public gin::Wrappable<SystemPreferences>,
      public gin_helper::EventEmitterMixin<SystemPreferences>
#if BUILDFLAG(IS_WIN)
    ,
      public BrowserObserver,
      public gfx::SysColorChangeListener
#endif
{
 public:
  static gin::Handle<SystemPreferences> Create(v8::Isolate* isolate);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  std::string GetAccentColor();
  std::string GetColor(gin_helper::ErrorThrower thrower,
                       const std::string& color);
  std::string GetMediaAccessStatus(gin_helper::ErrorThrower thrower,
                                   const std::string& media_type);
#endif
#if BUILDFLAG(IS_WIN)
  bool IsAeroGlassEnabled();

  void InitializeWindow();

  // gfx::SysColorChangeListener:
  void OnSysColorChange() override;

  // BrowserObserver:
  void OnFinishLaunching(base::Value::Dict launch_info) override;

#elif BUILDFLAG(IS_MAC)
  using NotificationCallback = base::RepeatingCallback<
      void(const std::string&, base::Value, const std::string&)>;

  void PostNotification(const std::string& name,
                        base::Value::Dict user_info,
                        gin::Arguments* args);
  int SubscribeNotification(v8::Local<v8::Value> maybe_name,
                            const NotificationCallback& callback);
  void UnsubscribeNotification(int id);
  void PostLocalNotification(const std::string& name,
                             base::Value::Dict user_info);
  int SubscribeLocalNotification(v8::Local<v8::Value> maybe_name,
                                 const NotificationCallback& callback);
  void UnsubscribeLocalNotification(int request_id);
  void PostWorkspaceNotification(const std::string& name,
                                 base::Value::Dict user_info);
  int SubscribeWorkspaceNotification(v8::Local<v8::Value> maybe_name,
                                     const NotificationCallback& callback);
  void UnsubscribeWorkspaceNotification(int request_id);
  v8::Local<v8::Value> GetUserDefault(v8::Isolate* isolate,
                                      const std::string& name,
                                      const std::string& type);
  void RegisterDefaults(gin::Arguments* args);
  void SetUserDefault(const std::string& name,
                      const std::string& type,
                      gin::Arguments* args);
  void RemoveUserDefault(const std::string& name);
  bool IsSwipeTrackingFromScrollEventsEnabled();

  std::string GetSystemColor(gin_helper::ErrorThrower thrower,
                             const std::string& color);

  bool CanPromptTouchID();
  v8::Local<v8::Promise> PromptTouchID(v8::Isolate* isolate,
                                       const std::string& reason);

  static bool IsTrustedAccessibilityClient(bool prompt);

  v8::Local<v8::Promise> AskForMediaAccess(v8::Isolate* isolate,
                                           const std::string& media_type);

  // TODO(MarshallOfSound): Write tests for these methods once we
  // are running tests on a Mojave machine
  v8::Local<v8::Value> GetEffectiveAppearance(v8::Isolate* isolate);
  v8::Local<v8::Value> GetAppLevelAppearance(v8::Isolate* isolate);
  void SetAppLevelAppearance(gin::Arguments* args);
#endif
  bool IsInvertedColorScheme();
  bool IsHighContrastColorScheme();
  v8::Local<v8::Value> GetAnimationSettings(v8::Isolate* isolate);

  // disable copy
  SystemPreferences(const SystemPreferences&) = delete;
  SystemPreferences& operator=(const SystemPreferences&) = delete;

 protected:
  SystemPreferences();
  ~SystemPreferences() override;

#if BUILDFLAG(IS_MAC)
  int DoSubscribeNotification(v8::Local<v8::Value> maybe_name,
                              const NotificationCallback& callback,
                              NotificationCenterKind kind);
  void DoUnsubscribeNotification(int request_id, NotificationCenterKind kind);
#endif

 private:
#if BUILDFLAG(IS_WIN)
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

  bool inverted_color_scheme_ = false;

  bool high_contrast_color_scheme_ = false;

  std::unique_ptr<gfx::ScopedSysColorChangeListener> color_change_listener_;
#endif
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_

// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_

#include <string>

#include "base/values.h"
#include "gin/per_isolate_data.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"

#if BUILDFLAG(IS_WIN)
#include "base/callback_list.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_observer.h"
#endif
#if BUILDFLAG(IS_LINUX)
#include "base/memory/raw_ptr.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"
#endif

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace electron::api {

#if BUILDFLAG(IS_MAC)
enum class NotificationCenterKind {
  kNSDistributedNotificationCenter = 0,
  kNSNotificationCenter,
  kNSWorkspaceNotificationCenter,
};
#endif

class SystemPreferences final
    : public gin::Wrappable<SystemPreferences>,
      public gin_helper::EventEmitterMixin<SystemPreferences>,
      public gin::PerIsolateData::DisposeObserver
#if BUILDFLAG(IS_WIN)
    ,
      public BrowserObserver
#elif BUILDFLAG(IS_LINUX)
    ,
      public ui::NativeThemeObserver
#endif
{
 public:
  static SystemPreferences* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "SystemPreferences"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override {}
  void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) override;
  void OnDisposed() override {}

  std::string GetAccentColor();
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  std::string GetColor(gin_helper::ErrorThrower thrower,
                       const std::string& color);
  std::string GetMediaAccessStatus(gin_helper::ErrorThrower thrower,
                                   const std::string& media_type);
#endif
#if BUILDFLAG(IS_WIN)
  void InitializeWindow();

  // Called by `hwnd_subscription_`.
  void OnWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // BrowserObserver:
  void OnFinishLaunching(base::DictValue launch_info) override;

#elif BUILDFLAG(IS_MAC)
  using NotificationCallback = base::RepeatingCallback<
      void(const std::string&, base::Value, const std::string&)>;

  void PostNotification(const std::string& name,
                        base::DictValue user_info,
                        gin::Arguments* args);
  int SubscribeNotification(v8::Local<v8::Value> maybe_name,
                            const NotificationCallback& callback);
  void UnsubscribeNotification(int id);
  void PostLocalNotification(const std::string& name,
                             base::DictValue user_info);
  int SubscribeLocalNotification(v8::Local<v8::Value> maybe_name,
                                 const NotificationCallback& callback);
  void UnsubscribeLocalNotification(int request_id);
  void PostWorkspaceNotification(const std::string& name,
                                 base::DictValue user_info);
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
  bool AccessibilityDisplayShouldReduceTransparency();

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

#elif BUILDFLAG(IS_LINUX)
  // ui::NativeThemeObserver:
  void OnNativeThemeUpdated(ui::NativeTheme* theme) override;
#endif
  v8::Local<v8::Value> GetAnimationSettings(v8::Isolate* isolate);

  // disable copy
  SystemPreferences(const SystemPreferences&) = delete;
  SystemPreferences& operator=(const SystemPreferences&) = delete;

  // Public for cppgc::MakeGarbageCollected.
  explicit SystemPreferences(v8::Isolate* isolate);
  ~SystemPreferences() override;

#if BUILDFLAG(IS_MAC)
  int DoSubscribeNotification(v8::Local<v8::Value> maybe_name,
                              const NotificationCallback& callback,
                              NotificationCenterKind kind);
  void DoUnsubscribeNotification(int request_id, NotificationCenterKind kind);
  void ClearNotificationSubscriptions();
#endif

 private:
  void Dispose();

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
  ATOM atom_ = 0;

  // The handle of the module that contains the window procedure of |window_|.
  HMODULE instance_ = nullptr;

  // The window used for processing events.
  HWND window_ = nullptr;

  std::string current_color_;

  // Color/high contrast mode change observer.
  base::CallbackListSubscription hwnd_subscription_;
#endif
#if BUILDFLAG(IS_LINUX)
  void OnNativeThemeUpdatedOnUI();

  raw_ptr<ui::NativeTheme> ui_theme_;
  std::string current_accent_color_;
  gin::WeakCellFactory<SystemPreferences> weak_factory_{this};
#endif
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_

// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_
#define ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "base/callback.h"
#include "base/values.h"
#include "native_mate/handle.h"

#if defined(OS_WIN)
#include "ui/gfx/sys_color_change_listener.h"
#endif

namespace base {
class DictionaryValue;
}

namespace atom {

namespace api {

class SystemPreferences : public mate::EventEmitter<SystemPreferences>
#if defined(OS_WIN)
    , public gfx::SysColorChangeListener
#endif
  {
 public:
  static mate::Handle<SystemPreferences> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if defined(OS_WIN)
  bool IsAeroGlassEnabled();

  typedef HRESULT (STDAPICALLTYPE *DwmGetColorizationColor)(DWORD *, BOOL *);
  DwmGetColorizationColor dwmGetColorizationColor =
    (DwmGetColorizationColor) GetProcAddress(LoadLibraryW(L"dwmapi.dll"),
                                            "DwmGetColorizationColor");

  std::string GetAccentColor();
  std::string GetColor(const std::string& color, mate::Arguments* args);

  void InitializeWindow();

  // gfx::SysColorChangeListener:
  void OnSysColorChange() override;

#elif defined(OS_MACOSX)
  using NotificationCallback = base::Callback<
    void(const std::string&, const base::DictionaryValue&)>;

  void PostNotification(const std::string& name,
                        const base::DictionaryValue& user_info);
  void PostLocalNotification(const std::string& name,
                             const base::DictionaryValue& user_info);
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
  bool IsInvertedColorScheme();

 protected:
  explicit SystemPreferences(v8::Isolate* isolate);
  ~SystemPreferences() override;

#if defined(OS_MACOSX)
  void DoPostNotification(const std::string& name,
                          const base::DictionaryValue& user_info,
                          bool is_local);
  int DoSubscribeNotification(const std::string& name,
                              const NotificationCallback& callback,
                              bool is_local);
  void DoUnsubscribeNotification(int request_id, bool is_local);
#endif

 private:
#if defined(OS_WIN)
  // Static callback invoked when a message comes in to our messaging window.
  static LRESULT CALLBACK
      WndProcStatic(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  LRESULT CALLBACK
      WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // The window class of |window_|.
  ATOM atom_;

  // The handle of the module that contains the window procedure of |window_|.
  HMODULE instance_;

  // The window used for processing events.
  HWND window_;

  std::string current_color_;

  bool invertered_color_scheme_;

  gfx::ScopedSysColorChangeListener color_change_listener_;
#endif
  DISALLOW_COPY_AND_ASSIGN(SystemPreferences);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SYSTEM_PREFERENCES_H_

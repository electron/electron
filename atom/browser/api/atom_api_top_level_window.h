// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_TOP_LEVEL_WINDOW_H_
#define ATOM_BROWSER_API_ATOM_API_TOP_LEVEL_WINDOW_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/native_window.h"
#include "atom/browser/native_window_observer.h"
#include "atom/common/api/atom_api_native_image.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class View;

class TopLevelWindow : public mate::TrackableObject<TopLevelWindow>,
                       public NativeWindowObserver {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  base::WeakPtr<TopLevelWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  NativeWindow* window() const { return window_.get(); }

 protected:
  // Common constructor.
  TopLevelWindow(v8::Isolate* isolate, const mate::Dictionary& options);
  // Creating independent TopLevelWindow instance.
  TopLevelWindow(v8::Isolate* isolate,
                 v8::Local<v8::Object> wrapper,
                 const mate::Dictionary& options);
  ~TopLevelWindow() override;

  // TrackableObject:
  void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) override;

  // NativeWindowObserver:
  void WillCloseWindow(bool* prevent_default) override;
  void OnWindowClosed() override;
  void OnWindowEndSession() override;
  void OnWindowBlur() override;
  void OnWindowFocus() override;
  void OnWindowShow() override;
  void OnWindowHide() override;
  void OnWindowMaximize() override;
  void OnWindowUnmaximize() override;
  void OnWindowMinimize() override;
  void OnWindowRestore() override;
  void OnWindowResize() override;
  void OnWindowMove() override;
  void OnWindowMoved() override;
  void OnWindowScrollTouchBegin() override;
  void OnWindowScrollTouchEnd() override;
  void OnWindowSwipe(const std::string& direction) override;
  void OnWindowSheetBegin() override;
  void OnWindowSheetEnd() override;
  void OnWindowEnterFullScreen() override;
  void OnWindowLeaveFullScreen() override;
  void OnWindowEnterHtmlFullScreen() override;
  void OnWindowLeaveHtmlFullScreen() override;
  void OnExecuteWindowsCommand(const std::string& command_name) override;
  void OnTouchBarItemResult(const std::string& item_id,
                            const base::DictionaryValue& details) override;
  void OnNewWindowForTab() override;
#if defined(OS_WIN)
  void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) override;
#endif

  // Public APIs of NativeWindow.
  void SetContentView(mate::Handle<View> view);
  void Close();
  virtual void Focus();
  virtual void Blur();
  bool IsFocused();
  void Show();
  void ShowInactive();
  void Hide();
  bool IsVisible();
  bool IsEnabled();
  void SetEnabled(bool enable);
  void Maximize();
  void Unmaximize();
  bool IsMaximized();
  void Minimize();
  void Restore();
  bool IsMinimized();
  void SetFullScreen(bool fullscreen);
  bool IsFullscreen();
  void SetBounds(const gfx::Rect& bounds, mate::Arguments* args);
  gfx::Rect GetBounds();
  void SetSize(int width, int height, mate::Arguments* args);
  std::vector<int> GetSize();
  void SetContentSize(int width, int height, mate::Arguments* args);
  std::vector<int> GetContentSize();
  void SetContentBounds(const gfx::Rect& bounds, mate::Arguments* args);
  gfx::Rect GetContentBounds();
  void SetMinimumSize(int width, int height);
  std::vector<int> GetMinimumSize();
  void SetMaximumSize(int width, int height);
  std::vector<int> GetMaximumSize();
  void SetSheetOffset(double offsetY, mate::Arguments* args);
  void SetResizable(bool resizable);
  bool IsResizable();
  void SetMovable(bool movable);
#if defined(OS_WIN) || defined(OS_MACOSX)
  void MoveTop();
#endif
  bool IsMovable();
  void SetMinimizable(bool minimizable);
  bool IsMinimizable();
  void SetMaximizable(bool maximizable);
  bool IsMaximizable();
  void SetFullScreenable(bool fullscreenable);
  bool IsFullScreenable();
  void SetClosable(bool closable);
  bool IsClosable();
  void SetAlwaysOnTop(bool top, mate::Arguments* args);
  bool IsAlwaysOnTop();
  void Center();
  void SetPosition(int x, int y, mate::Arguments* args);
  std::vector<int> GetPosition();
  void SetTitle(const std::string& title);
  std::string GetTitle();
  void FlashFrame(bool flash);
  void SetSkipTaskbar(bool skip);
  void SetSimpleFullScreen(bool simple_fullscreen);
  bool IsSimpleFullScreen();
  void SetKiosk(bool kiosk);
  bool IsKiosk();
  virtual void SetBackgroundColor(const std::string& color_name);
  void SetHasShadow(bool has_shadow);
  bool HasShadow();
  void SetOpacity(const double opacity);
  double GetOpacity();
  void SetShape(const std::vector<gfx::Rect>& rects);
  void SetRepresentedFilename(const std::string& filename);
  std::string GetRepresentedFilename();
  void SetDocumentEdited(bool edited);
  bool IsDocumentEdited();
  void SetIgnoreMouseEvents(bool ignore, mate::Arguments* args);
  void SetContentProtection(bool enable);
  void SetFocusable(bool focusable);
  void SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> menu);
  void SetParentWindow(v8::Local<v8::Value> value, mate::Arguments* args);
  virtual void SetBrowserView(v8::Local<v8::Value> value);
  v8::Local<v8::Value> GetNativeWindowHandle();
  void SetProgressBar(double progress, mate::Arguments* args);
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description);
  void SetVisibleOnAllWorkspaces(bool visible);
  bool IsVisibleOnAllWorkspaces();
  void SetAutoHideCursor(bool auto_hide);
  virtual void SetVibrancy(v8::Isolate* isolate, v8::Local<v8::Value> value);
  void SetTouchBar(const std::vector<mate::PersistentDictionary>& items);
  void RefreshTouchBarItem(const std::string& item_id);
  void SetEscapeTouchBarItem(const mate::PersistentDictionary& item);
  void SelectPreviousTab();
  void SelectNextTab();
  void MergeAllWindows();
  void MoveTabToNewWindow();
  void ToggleTabBar();
  void AddTabbedWindow(NativeWindow* window, mate::Arguments* args);
  void SetAutoHideMenuBar(bool auto_hide);
  bool IsMenuBarAutoHide();
  void SetMenuBarVisibility(bool visible);
  bool IsMenuBarVisible();
  void SetAspectRatio(double aspect_ratio, mate::Arguments* args);
  void PreviewFile(const std::string& path, mate::Arguments* args);
  void CloseFilePreview();

  // Public getters of NativeWindow.
  v8::Local<v8::Value> GetContentView() const;
  v8::Local<v8::Value> GetParentWindow() const;
  std::vector<v8::Local<v8::Object>> GetChildWindows() const;
  v8::Local<v8::Value> GetBrowserView() const;
  bool IsModal() const;

  // Extra APIs added in JS.
  bool SetThumbarButtons(mate::Arguments* args);
#if defined(TOOLKIT_VIEWS)
  void SetIcon(mate::Handle<NativeImage> icon);
#endif
#if defined(OS_WIN)
  typedef base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>
      MessageCallback;
  bool HookWindowMessage(UINT message, const MessageCallback& callback);
  bool IsWindowMessageHooked(UINT message);
  void UnhookWindowMessage(UINT message);
  void UnhookAllWindowMessages();
  bool SetThumbnailClip(const gfx::Rect& region);
  bool SetThumbnailToolTip(const std::string& tooltip);
  void SetAppDetails(const mate::Dictionary& options);
#endif
  int32_t GetID() const;

  // Helpers.

  // Remove BrowserView.
  void ResetBrowserView();

  // Remove this window from parent window's |child_windows_|.
  void RemoveFromParentChildWindows();

  template<typename... Args>
  void EmitEventSoon(base::StringPiece eventName) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(base::IgnoreResult(&TopLevelWindow::Emit<Args...>),
                       weak_factory_.GetWeakPtr(), eventName));
  }

#if defined(OS_WIN)
  typedef std::map<UINT, MessageCallback> MessageCallbackMap;
  MessageCallbackMap messages_callback_map_;
#endif

  v8::Global<v8::Value> content_view_;
  v8::Global<v8::Value> browser_view_;
  v8::Global<v8::Value> menu_;
  v8::Global<v8::Value> parent_window_;
  KeyWeakMap<int> child_windows_;

  std::unique_ptr<NativeWindow> window_;

  base::WeakPtrFactory<TopLevelWindow> weak_factory_;
};

}  // namespace api

}  // namespace atom

namespace mate {

template <>
struct Converter<atom::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::NativeWindow** out) {
    // null would be tranfered to NULL.
    if (val->IsNull()) {
      *out = NULL;
      return true;
    }

    atom::api::TopLevelWindow* window;
    if (!Converter<atom::api::TopLevelWindow*>::FromV8(isolate, val, &window))
      return false;
    *out = window->window();
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_ATOM_API_TOP_LEVEL_WINDOW_H_

// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BASE_WINDOW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BASE_WINDOW_H_

#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "content/public/browser/browser_thread.h"
#include "shell/browser/native_window_observer.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_helper/trackable_object.h"
#include "v8/include/cppgc/persistent.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class PersistentDictionary;
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron {

class NativeWindow;

namespace api {

class Menu;
class View;

class BaseWindow : public gin_helper::TrackableObject<BaseWindow>,
                   private NativeWindowObserver {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);
  // The constructor's template, created on first use; BrowserWindow inherits
  // from it.
  static v8::Local<v8::FunctionTemplate> GetConstructorTemplate(
      v8::Isolate* isolate);

  // Clears window state from the Local State JSON file in
  // app.getPath('userData') via PrefService.
  static void ClearPersistedState(const std::string& window_name);

  static bool IsWindowNameValid(const gin_helper::Dictionary& options,
                                std::string* error_message);

  // False when the options name an app shim (macOS) that can't host a window.
  static bool IsAppShimValid(const gin_helper::Dictionary& options,
                             std::string* error_message);
  // Returns false, with the reason, when the `parent` in `options` is a
  // window that can't have children.
  static bool IsParentValid(const gin_helper::Dictionary& options,
                            std::string* error_message);

  const NativeWindow* window() const { return window_.get(); }
  NativeWindow* window() { return window_.get(); }

  static BaseWindow* GetFocusedWindow();
  // Null unless |value| is a live BaseWindow.
  static BaseWindow* FromValue(v8::Isolate* isolate,
                               v8::Local<v8::Value> value);
  // |window| may be dangling.
  static bool IsLive(const BaseWindow* window);
  void SetMenuNatively(Menu* menu);
  void RemoveMenu();

 protected:
  // Common constructor.
  BaseWindow(v8::Isolate* isolate, const gin_helper::Dictionary& options);
  // Creating independent BaseWindow instance.
  BaseWindow(gin::Arguments* args, const gin_helper::Dictionary& options);
  ~BaseWindow() override;

  // gin_helper::WrappableBase:
  void OnWrapped(v8::Isolate* isolate) override;

  // TrackableObject:
  void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) override;

  // NativeWindowObserver:
  void WillCloseWindow(bool* prevent_default) override;
  void OnWindowClosed() override;
  void OnWindowQueryEndSession(const std::vector<std::string>& reasons,
                               bool* prevent_default) override;
  void OnWindowEndSession(const std::vector<std::string>& reasons) override;
  void OnWindowBlur() override;
  void OnWindowFocus() override;
  void OnWindowShow() override;
  void OnWindowHide() override;
  void OnWindowMaximize() override;
  void OnWindowUnmaximize() override;
  void OnWindowMinimize() override;
  void OnWindowRestore() override;
  void OnWindowWillResize(const gfx::Rect& new_bounds,
                          gfx::ResizeEdge edge,
                          bool* prevent_default) override;
  void OnWindowResize() override;
  void OnWindowResized() override;
  void OnWindowWillMove(const gfx::Rect& new_bounds,
                        bool* prevent_default) override;
  void OnWindowMove() override;
  void OnWindowMoved() override;
  void OnWindowSwipe(const std::string& direction) override;
  void OnWindowRotateGesture(float rotation) override;
  void OnWindowSheetBegin() override;
  void OnWindowSheetEnd() override;
  void OnWindowIsKeyChanged(bool is_key) override;
  void OnWindowEnterFullScreen() override;
  void OnWindowLeaveFullScreen() override;
  void OnWindowEnterHtmlFullScreen() override;
  void OnWindowLeaveHtmlFullScreen() override;
  void OnWindowAlwaysOnTopChanged(bool is_always_on_top) override;
  void OnExecuteAppCommand(std::string_view command_name) override;
  void OnTouchBarItemResult(const std::string& item_id,
                            const base::DictValue& details) override;
  void OnNewWindowForTab() override;
  void OnSystemContextMenu(int x, int y, bool* prevent_default) override;
  void OnWindowStateRestored() override;
#if BUILDFLAG(IS_WIN)
  void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) override;
#endif

  // The NativeWindow APIs that need a wrapper here: they adapt arguments or
  // results, keep state, or are overridden by BrowserWindow. The rest of the
  // JavaScript API binds NativeWindow's methods directly in BuildPrototype().
  void SetContentView(View* view);
  virtual void CloseImmediately();
  virtual void Focus();
  virtual void Blur();
  virtual void Show();
  virtual void ShowInactive();
  void SetBounds(v8::Local<v8::Object> bounds, gin::Arguments* args);
  void SetSize(int width, int height, gin::Arguments* args);
  std::array<int, 2U> GetSize() const;
  void SetContentSize(int width, int height, gin::Arguments* args);
  std::array<int, 2U> GetContentSize() const;
  void SetContentBounds(const gfx::Rect& bounds, gin::Arguments* args);
  void SetMinimumSize(int width, int height);
  std::array<int, 2U> GetMinimumSize() const;
  void SetMaximumSize(int width, int height);
  std::array<int, 2U> GetMaximumSize() const;
  void SetSheetOffset(double offsetY, gin::Arguments* args);
  void MoveAbove(const std::string& sourceId, gin::Arguments* args);
  void SetAlwaysOnTop(bool top, gin::Arguments* args);
  bool IsAlwaysOnTop() const;
  void SetPosition(int x, int y, gin::Arguments* args);
  std::array<int, 2U> GetPosition() const;
  void SetTitle(const std::string& title);
  void SetTitleFromPage(const std::string& title);
  bool SetTitleFromPageIfNotSetFromApi(const std::string& title);
  virtual void SetBackgroundColor(const std::string& color_name);
  std::string GetBackgroundColor() const;
  void SetIgnoreMouseEvents(bool ignore, gin::Arguments* args);
  void SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> menu);
  void SetParentWindow(v8::Local<v8::Value> value, gin::Arguments* args);
  std::string GetMediaSourceId() const;
  v8::Local<v8::Value> GetNativeWindowHandle();
  void SetProgressBar(double progress, gin::Arguments* args);
  void SetVisibleOnAllWorkspaces(bool visible, gin::Arguments* args);
  virtual void SetVibrancy(v8::Isolate* isolate,
                           v8::Local<v8::Value> value,
                           gin::Arguments* args);
  virtual void SetBackgroundMaterial(const std::string& material);

  void AddTabbedWindow(NativeWindow* window, gin::Arguments* args);
  v8::Local<v8::Value> GetTabbingIdentifier();
  void SetAspectRatio(double aspect_ratio, gin::Arguments* args);
  void PreviewFile(const std::string& path, gin::Arguments* args);

  // Public getters of NativeWindow.
  v8::Local<v8::Value> GetContentView() const;
  BaseWindow* GetParentWindow() const;
  std::vector<BaseWindow*> GetChildWindows() const;

  // Extra APIs added in JS.
  bool SetThumbarButtons(gin::Arguments* args);
#if defined(TOOLKIT_VIEWS)
  void SetIcon(v8::Isolate* isolate, v8::Local<v8::Value> icon);
  void SetIconImpl(v8::Isolate* isolate,
                   v8::Local<v8::Value> icon,
                   NativeImage::OnConvertError on_error);
#endif
#if BUILDFLAG(IS_WIN)
  typedef base::RepeatingCallback<void(v8::Local<v8::Value>,
                                       v8::Local<v8::Value>)>
      MessageCallback;
  bool HookWindowMessage(UINT message, const MessageCallback& callback);
  bool IsWindowMessageHooked(UINT message);
  void UnhookWindowMessage(UINT message);
  void UnhookAllWindowMessages();
  bool SetThumbnailClip(const gfx::Rect& region);
  bool SetThumbnailToolTip(const std::string& tooltip);
  void SetAppDetails(const gin_helper::Dictionary& options);
  void SetAccentColor(gin::Arguments* args);
  v8::Local<v8::Value> GetAccentColor() const;
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
  void SetTitleBarOverlay(const gin_helper::Dictionary& options,
                          gin::Arguments* args);
#endif
  [[nodiscard]] constexpr int32_t GetID() const { return weak_map_id(); }

 private:
  // Helpers.

  template <typename... Args>
  void EmitEventSoon(std::string_view eventName) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(base::IgnoreResult(&BaseWindow::Emit<Args...>),
                       weak_factory_.GetWeakPtr(), eventName));
  }

#if BUILDFLAG(IS_WIN)
  typedef std::map<UINT, MessageCallback> MessageCallbackMap;
  MessageCallbackMap messages_callback_map_;
#endif

  cppgc::Persistent<View> content_view_;
  cppgc::Persistent<Menu> menu_;
  v8::Global<v8::Value> parent_window_;

  std::unique_ptr<NativeWindow> window_;

  // Reference to JS wrapper to prevent garbage collection.
  v8::Global<v8::Value> self_ref_;

  bool title_set_from_api_ = false;

  base::WeakPtrFactory<BaseWindow> weak_factory_{this};
};

}  // namespace api
}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BASE_WINDOW_H_

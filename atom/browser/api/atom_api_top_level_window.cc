// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_top_level_window.h"

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_browser_view.h"
#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/api/atom_api_view.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "electron/buildflags/buildflags.h"
#include "gin/converter.h"
#include "native_mate/handle.h"
#include "native_mate/persistent_dictionary.h"

#if defined(TOOLKIT_VIEWS)
#include "atom/browser/native_window_views.h"
#endif

#if defined(OS_WIN)
#include "atom/browser/ui/win/taskbar_host.h"
#include "ui/base/win/shell.h"
#endif

#include "atom/common/node_includes.h"

#if defined(OS_WIN)
namespace mate {

template <>
struct Converter<atom::TaskbarHost::ThumbarButton> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     atom::TaskbarHost::ThumbarButton* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.Get("click", &(out->clicked_callback));
    dict.Get("tooltip", &(out->tooltip));
    dict.Get("flags", &out->flags);
    return dict.Get("icon", &(out->icon));
  }
};

}  // namespace mate
#endif

namespace atom {

namespace api {

namespace {

// Converts binary data to Buffer.
v8::Local<v8::Value> ToBuffer(v8::Isolate* isolate, void* val, int size) {
  auto buffer = node::Buffer::Copy(isolate, static_cast<char*>(val), size);
  if (buffer.IsEmpty())
    return v8::Null(isolate);
  else
    return buffer.ToLocalChecked();
}

}  // namespace

TopLevelWindow::TopLevelWindow(v8::Isolate* isolate,
                               const mate::Dictionary& options)
    : weak_factory_(this) {
  // The parent window.
  mate::Handle<TopLevelWindow> parent;
  if (options.Get("parent", &parent) && !parent.IsEmpty())
    parent_window_.Reset(isolate, parent.ToV8());

#if BUILDFLAG(ENABLE_OSR)
  // Offscreen windows are always created frameless.
  mate::Dictionary web_preferences;
  bool offscreen;
  if (options.Get(options::kWebPreferences, &web_preferences) &&
      web_preferences.Get(options::kOffscreen, &offscreen) && offscreen) {
    const_cast<mate::Dictionary&>(options).Set(options::kFrame, false);
  }
#endif

  // Creates NativeWindow.
  window_.reset(NativeWindow::Create(
      options, parent.IsEmpty() ? nullptr : parent->window_.get()));
  window_->AddObserver(this);

#if defined(TOOLKIT_VIEWS)
  // Sets the window icon.
  mate::Handle<NativeImage> icon;
  if (options.Get(options::kIcon, &icon) && !icon.IsEmpty())
    SetIcon(icon);
#endif
}

TopLevelWindow::TopLevelWindow(v8::Isolate* isolate,
                               v8::Local<v8::Object> wrapper,
                               const mate::Dictionary& options)
    : TopLevelWindow(isolate, options) {
  InitWith(isolate, wrapper);
  // Init window after everything has been setup.
  window()->InitFromOptions(options);
}

TopLevelWindow::~TopLevelWindow() {
  if (!window_->IsClosed())
    window_->CloseImmediately();

  // Destroy the native window in next tick because the native code might be
  // iterating all windows.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, window_.release());
}

void TopLevelWindow::InitWith(v8::Isolate* isolate,
                              v8::Local<v8::Object> wrapper) {
  AttachAsUserData(window_.get());
  mate::TrackableObject<TopLevelWindow>::InitWith(isolate, wrapper);

  // We can only append this window to parent window's child windows after this
  // window's JS wrapper gets initialized.
  if (!parent_window_.IsEmpty()) {
    mate::Handle<TopLevelWindow> parent;
    mate::ConvertFromV8(isolate, GetParentWindow(), &parent);
    DCHECK(!parent.IsEmpty());
    parent->child_windows_.Set(isolate, weak_map_id(), wrapper);
  }
}

void TopLevelWindow::WillCloseWindow(bool* prevent_default) {
  if (Emit("close")) {
    *prevent_default = true;
  }
}

void TopLevelWindow::OnWindowClosed() {
  // Invalidate weak ptrs before the Javascript object is destroyed,
  // there might be some delayed emit events which shouldn't be
  // triggered after this.
  weak_factory_.InvalidateWeakPtrs();

  RemoveFromWeakMap();
  window_->RemoveObserver(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  Emit("closed");

  RemoveFromParentChildWindows();
  TopLevelWindow::ResetBrowserViews();

  // Destroy the native class when window is closed.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, GetDestroyClosure());
}

void TopLevelWindow::OnWindowEndSession() {
  Emit("session-end");
}

void TopLevelWindow::OnWindowBlur() {
  EmitEventSoon("blur");
}

void TopLevelWindow::OnWindowFocus() {
  EmitEventSoon("focus");
}

void TopLevelWindow::OnWindowShow() {
  Emit("show");
}

void TopLevelWindow::OnWindowHide() {
  Emit("hide");
}

void TopLevelWindow::OnWindowMaximize() {
  Emit("maximize");
}

void TopLevelWindow::OnWindowUnmaximize() {
  Emit("unmaximize");
}

void TopLevelWindow::OnWindowMinimize() {
  Emit("minimize");
}

void TopLevelWindow::OnWindowRestore() {
  Emit("restore");
}

void TopLevelWindow::OnWindowWillResize(const gfx::Rect& new_bounds,
                                        bool* prevent_default) {
  if (Emit("will-resize", new_bounds)) {
    *prevent_default = true;
  }
}

void TopLevelWindow::OnWindowResize() {
  Emit("resize");
}

void TopLevelWindow::OnWindowWillMove(const gfx::Rect& new_bounds,
                                      bool* prevent_default) {
  if (Emit("will-move", new_bounds)) {
    *prevent_default = true;
  }
}

void TopLevelWindow::OnWindowMove() {
  Emit("move");
}

void TopLevelWindow::OnWindowMoved() {
  Emit("moved");
}

void TopLevelWindow::OnWindowEnterFullScreen() {
  Emit("enter-full-screen");
}

void TopLevelWindow::OnWindowLeaveFullScreen() {
  Emit("leave-full-screen");
}

void TopLevelWindow::OnWindowScrollTouchBegin() {
  Emit("scroll-touch-begin");
}

void TopLevelWindow::OnWindowScrollTouchEnd() {
  Emit("scroll-touch-end");
}

void TopLevelWindow::OnWindowSwipe(const std::string& direction) {
  Emit("swipe", direction);
}

void TopLevelWindow::OnWindowSheetBegin() {
  Emit("sheet-begin");
}

void TopLevelWindow::OnWindowSheetEnd() {
  Emit("sheet-end");
}

void TopLevelWindow::OnWindowEnterHtmlFullScreen() {
  Emit("enter-html-full-screen");
}

void TopLevelWindow::OnWindowLeaveHtmlFullScreen() {
  Emit("leave-html-full-screen");
}

void TopLevelWindow::OnWindowAlwaysOnTopChanged() {
  Emit("always-on-top-changed", IsAlwaysOnTop());
}

void TopLevelWindow::OnExecuteAppCommand(const std::string& command_name) {
  Emit("app-command", command_name);
}

void TopLevelWindow::OnTouchBarItemResult(
    const std::string& item_id,
    const base::DictionaryValue& details) {
  Emit("-touch-bar-interaction", item_id, details);
}

void TopLevelWindow::OnNewWindowForTab() {
  Emit("new-window-for-tab");
}

#if defined(OS_WIN)
void TopLevelWindow::OnWindowMessage(UINT message,
                                     WPARAM w_param,
                                     LPARAM l_param) {
  if (IsWindowMessageHooked(message)) {
    messages_callback_map_[message].Run(
        ToBuffer(isolate(), static_cast<void*>(&w_param), sizeof(WPARAM)),
        ToBuffer(isolate(), static_cast<void*>(&l_param), sizeof(LPARAM)));
  }
}
#endif

void TopLevelWindow::SetContentView(mate::Handle<View> view) {
  ResetBrowserViews();
  content_view_.Reset(isolate(), view.ToV8());
  window_->SetContentView(view->view());
}

void TopLevelWindow::Close() {
  window_->Close();
}

void TopLevelWindow::Focus() {
  window_->Focus(true);
}

void TopLevelWindow::Blur() {
  window_->Focus(false);
}

bool TopLevelWindow::IsFocused() {
  return window_->IsFocused();
}

void TopLevelWindow::Show() {
  window_->Show();
}

void TopLevelWindow::ShowInactive() {
  // This method doesn't make sense for modal window.
  if (IsModal())
    return;
  window_->ShowInactive();
}

void TopLevelWindow::Hide() {
  window_->Hide();
}

bool TopLevelWindow::IsVisible() {
  return window_->IsVisible();
}

bool TopLevelWindow::IsEnabled() {
  return window_->IsEnabled();
}

void TopLevelWindow::SetEnabled(bool enable) {
  window_->SetEnabled(enable);
}

void TopLevelWindow::Maximize() {
  window_->Maximize();
}

void TopLevelWindow::Unmaximize() {
  window_->Unmaximize();
}

bool TopLevelWindow::IsMaximized() {
  return window_->IsMaximized();
}

void TopLevelWindow::Minimize() {
  window_->Minimize();
}

void TopLevelWindow::Restore() {
  window_->Restore();
}

bool TopLevelWindow::IsMinimized() {
  return window_->IsMinimized();
}

void TopLevelWindow::SetFullScreen(bool fullscreen) {
  window_->SetFullScreen(fullscreen);
}

bool TopLevelWindow::IsFullscreen() {
  return window_->IsFullscreen();
}

void TopLevelWindow::SetBounds(const gfx::Rect& bounds, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetBounds(bounds, animate);
}

gfx::Rect TopLevelWindow::GetBounds() {
  return window_->GetBounds();
}

bool TopLevelWindow::IsNormal() {
  return window_->IsNormal();
}

gfx::Rect TopLevelWindow::GetNormalBounds() {
  return window_->GetNormalBounds();
}

void TopLevelWindow::SetContentBounds(const gfx::Rect& bounds,
                                      mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentBounds(bounds, animate);
}

gfx::Rect TopLevelWindow::GetContentBounds() {
  return window_->GetContentBounds();
}

void TopLevelWindow::SetSize(int width, int height, mate::Arguments* args) {
  bool animate = false;
  gfx::Size size = window_->GetMinimumSize();
  size.SetToMax(gfx::Size(width, height));
  args->GetNext(&animate);
  window_->SetSize(size, animate);
}

std::vector<int> TopLevelWindow::GetSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void TopLevelWindow::SetContentSize(int width,
                                    int height,
                                    mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentSize(gfx::Size(width, height), animate);
}

std::vector<int> TopLevelWindow::GetContentSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetContentSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void TopLevelWindow::SetMinimumSize(int width, int height) {
  window_->SetMinimumSize(gfx::Size(width, height));
}

std::vector<int> TopLevelWindow::GetMinimumSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetMinimumSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void TopLevelWindow::SetMaximumSize(int width, int height) {
  window_->SetMaximumSize(gfx::Size(width, height));
}

std::vector<int> TopLevelWindow::GetMaximumSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetMaximumSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void TopLevelWindow::SetSheetOffset(double offsetY, mate::Arguments* args) {
  double offsetX = 0.0;
  args->GetNext(&offsetX);
  window_->SetSheetOffset(offsetX, offsetY);
}

void TopLevelWindow::SetResizable(bool resizable) {
  window_->SetResizable(resizable);
}

bool TopLevelWindow::IsResizable() {
  return window_->IsResizable();
}

void TopLevelWindow::SetMovable(bool movable) {
  window_->SetMovable(movable);
}

bool TopLevelWindow::IsMovable() {
  return window_->IsMovable();
}

void TopLevelWindow::SetMinimizable(bool minimizable) {
  window_->SetMinimizable(minimizable);
}

bool TopLevelWindow::IsMinimizable() {
  return window_->IsMinimizable();
}

void TopLevelWindow::SetMaximizable(bool maximizable) {
  window_->SetMaximizable(maximizable);
}

bool TopLevelWindow::IsMaximizable() {
  return window_->IsMaximizable();
}

void TopLevelWindow::SetFullScreenable(bool fullscreenable) {
  window_->SetFullScreenable(fullscreenable);
}

bool TopLevelWindow::IsFullScreenable() {
  return window_->IsFullScreenable();
}

void TopLevelWindow::SetClosable(bool closable) {
  window_->SetClosable(closable);
}

bool TopLevelWindow::IsClosable() {
  return window_->IsClosable();
}

void TopLevelWindow::SetAlwaysOnTop(bool top, mate::Arguments* args) {
  std::string level = "floating";
  int relative_level = 0;
  args->GetNext(&level);
  args->GetNext(&relative_level);

  std::string error;
  window_->SetAlwaysOnTop(top, level, relative_level, &error);

  if (!error.empty())
    args->ThrowError(error);
}

bool TopLevelWindow::IsAlwaysOnTop() {
  return window_->IsAlwaysOnTop();
}

void TopLevelWindow::Center() {
  window_->Center();
}

void TopLevelWindow::SetPosition(int x, int y, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetPosition(gfx::Point(x, y), animate);
}

std::vector<int> TopLevelWindow::GetPosition() {
  std::vector<int> result(2);
  gfx::Point pos = window_->GetPosition();
  result[0] = pos.x();
  result[1] = pos.y();
  return result;
}

#if defined(OS_WIN) || defined(OS_MACOSX)
void TopLevelWindow::MoveTop() {
  window_->MoveTop();
}
#endif

void TopLevelWindow::SetTitle(const std::string& title) {
  window_->SetTitle(title);
}

std::string TopLevelWindow::GetTitle() {
  return window_->GetTitle();
}

void TopLevelWindow::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

void TopLevelWindow::SetSkipTaskbar(bool skip) {
  window_->SetSkipTaskbar(skip);
}

void TopLevelWindow::SetSimpleFullScreen(bool simple_fullscreen) {
  window_->SetSimpleFullScreen(simple_fullscreen);
}

bool TopLevelWindow::IsSimpleFullScreen() {
  return window_->IsSimpleFullScreen();
}

void TopLevelWindow::SetKiosk(bool kiosk) {
  window_->SetKiosk(kiosk);
}

bool TopLevelWindow::IsKiosk() {
  return window_->IsKiosk();
}

void TopLevelWindow::SetBackgroundColor(const std::string& color_name) {
  SkColor color = ParseHexColor(color_name);
  window_->SetBackgroundColor(color);
}

void TopLevelWindow::SetHasShadow(bool has_shadow) {
  window_->SetHasShadow(has_shadow);
}

bool TopLevelWindow::HasShadow() {
  return window_->HasShadow();
}

void TopLevelWindow::SetOpacity(const double opacity) {
  window_->SetOpacity(opacity);
}

double TopLevelWindow::GetOpacity() {
  return window_->GetOpacity();
}

void TopLevelWindow::SetShape(const std::vector<gfx::Rect>& rects) {
  window_->widget()->SetShape(std::make_unique<std::vector<gfx::Rect>>(rects));
}

void TopLevelWindow::SetRepresentedFilename(const std::string& filename) {
  window_->SetRepresentedFilename(filename);
}

std::string TopLevelWindow::GetRepresentedFilename() {
  return window_->GetRepresentedFilename();
}

void TopLevelWindow::SetDocumentEdited(bool edited) {
  window_->SetDocumentEdited(edited);
}

bool TopLevelWindow::IsDocumentEdited() {
  return window_->IsDocumentEdited();
}

void TopLevelWindow::SetIgnoreMouseEvents(bool ignore, mate::Arguments* args) {
  mate::Dictionary options;
  bool forward = false;
  args->GetNext(&options) && options.Get("forward", &forward);
  return window_->SetIgnoreMouseEvents(ignore, forward);
}

void TopLevelWindow::SetContentProtection(bool enable) {
  return window_->SetContentProtection(enable);
}

void TopLevelWindow::SetFocusable(bool focusable) {
  return window_->SetFocusable(focusable);
}

void TopLevelWindow::SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  mate::Handle<Menu> menu;
  if (value->IsObject() &&
      gin::V8ToString(
          isolate, value->ToObject(isolate)->GetConstructorName()) == "Menu" &&
      mate::ConvertFromV8(isolate, value, &menu) && !menu.IsEmpty()) {
    menu_.Reset(isolate, menu.ToV8());
    window_->SetMenu(menu->model());
  } else if (value->IsNull()) {
    menu_.Reset();
    window_->SetMenu(nullptr);
  } else {
    isolate->ThrowException(
        v8::Exception::TypeError(mate::StringToV8(isolate, "Invalid Menu")));
  }
}

void TopLevelWindow::RemoveMenu() {
  menu_.Reset();
  window_->SetMenu(nullptr);
}

void TopLevelWindow::SetParentWindow(v8::Local<v8::Value> value,
                                     mate::Arguments* args) {
  if (IsModal()) {
    args->ThrowError("Can not be called for modal window");
    return;
  }

  mate::Handle<TopLevelWindow> parent;
  if (value->IsNull() || value->IsUndefined()) {
    RemoveFromParentChildWindows();
    parent_window_.Reset();
    window_->SetParentWindow(nullptr);
  } else if (mate::ConvertFromV8(isolate(), value, &parent)) {
    RemoveFromParentChildWindows();
    parent_window_.Reset(isolate(), value);
    window_->SetParentWindow(parent->window_.get());
    parent->child_windows_.Set(isolate(), weak_map_id(), GetWrapper());
  } else {
    args->ThrowError("Must pass TopLevelWindow instance or null");
  }
}

void TopLevelWindow::SetBrowserView(v8::Local<v8::Value> value) {
  ResetBrowserViews();
  AddBrowserView(value);
}

void TopLevelWindow::AddBrowserView(v8::Local<v8::Value> value) {
  mate::Handle<BrowserView> browser_view;
  if (value->IsObject() &&
      mate::ConvertFromV8(isolate(), value, &browser_view)) {
    auto get_that_view = browser_views_.find(browser_view->weak_map_id());
    if (get_that_view == browser_views_.end()) {
      window_->AddBrowserView(browser_view->view());
      browser_view->web_contents()->SetOwnerWindow(window_.get());
      browser_views_[browser_view->weak_map_id()].Reset(isolate(), value);
    }
  }
}

void TopLevelWindow::RemoveBrowserView(v8::Local<v8::Value> value) {
  mate::Handle<BrowserView> browser_view;
  if (value->IsObject() &&
      mate::ConvertFromV8(isolate(), value, &browser_view)) {
    auto get_that_view = browser_views_.find(browser_view->weak_map_id());
    if (get_that_view != browser_views_.end()) {
      window_->RemoveBrowserView(browser_view->view());
      browser_view->web_contents()->SetOwnerWindow(nullptr);

      (*get_that_view).second.Reset(isolate(), value);
      browser_views_.erase(get_that_view);
    }
  }
}
v8::Local<v8::Value> TopLevelWindow::GetNativeWindowHandle() {
  // TODO(MarshallOfSound): Replace once
  // https://chromium-review.googlesource.com/c/chromium/src/+/1253094/ has
  // landed
  NativeWindowHandle handle = window_->GetNativeWindowHandle();
  return ToBuffer(isolate(), &handle, sizeof(handle));
}

void TopLevelWindow::SetProgressBar(double progress, mate::Arguments* args) {
  mate::Dictionary options;
  std::string mode;
  args->GetNext(&options) && options.Get("mode", &mode);

  NativeWindow::ProgressState state = NativeWindow::PROGRESS_NORMAL;
  if (mode == "error")
    state = NativeWindow::PROGRESS_ERROR;
  else if (mode == "paused")
    state = NativeWindow::PROGRESS_PAUSED;
  else if (mode == "indeterminate")
    state = NativeWindow::PROGRESS_INDETERMINATE;
  else if (mode == "none")
    state = NativeWindow::PROGRESS_NONE;

  window_->SetProgressBar(progress, state);
}

void TopLevelWindow::SetOverlayIcon(const gfx::Image& overlay,
                                    const std::string& description) {
  window_->SetOverlayIcon(overlay, description);
}

void TopLevelWindow::SetVisibleOnAllWorkspaces(bool visible,
                                               mate::Arguments* args) {
  mate::Dictionary options;
  bool visibleOnFullScreen = false;
  args->GetNext(&options) &&
      options.Get("visibleOnFullScreen", &visibleOnFullScreen);
  return window_->SetVisibleOnAllWorkspaces(visible, visibleOnFullScreen);
}

bool TopLevelWindow::IsVisibleOnAllWorkspaces() {
  return window_->IsVisibleOnAllWorkspaces();
}

void TopLevelWindow::SetAutoHideCursor(bool auto_hide) {
  window_->SetAutoHideCursor(auto_hide);
}

void TopLevelWindow::SetVibrancy(v8::Isolate* isolate,
                                 v8::Local<v8::Value> value) {
  std::string type = gin::V8ToString(isolate, value);
  window_->SetVibrancy(type);
}

void TopLevelWindow::SetTouchBar(
    const std::vector<mate::PersistentDictionary>& items) {
  window_->SetTouchBar(items);
}

void TopLevelWindow::RefreshTouchBarItem(const std::string& item_id) {
  window_->RefreshTouchBarItem(item_id);
}

void TopLevelWindow::SetEscapeTouchBarItem(
    const mate::PersistentDictionary& item) {
  window_->SetEscapeTouchBarItem(item);
}

void TopLevelWindow::SelectPreviousTab() {
  window_->SelectPreviousTab();
}

void TopLevelWindow::SelectNextTab() {
  window_->SelectNextTab();
}

void TopLevelWindow::MergeAllWindows() {
  window_->MergeAllWindows();
}

void TopLevelWindow::MoveTabToNewWindow() {
  window_->MoveTabToNewWindow();
}

void TopLevelWindow::ToggleTabBar() {
  window_->ToggleTabBar();
}

void TopLevelWindow::AddTabbedWindow(NativeWindow* window,
                                     mate::Arguments* args) {
  if (!window_->AddTabbedWindow(window))
    args->ThrowError("AddTabbedWindow cannot be called by a window on itself.");
}

void TopLevelWindow::SetWindowButtonVisibility(bool visible,
                                               mate::Arguments* args) {
  if (!window_->SetWindowButtonVisibility(visible)) {
    args->ThrowError("Not supported for this window");
  }
}

void TopLevelWindow::SetAutoHideMenuBar(bool auto_hide) {
  window_->SetAutoHideMenuBar(auto_hide);
}

bool TopLevelWindow::IsMenuBarAutoHide() {
  return window_->IsMenuBarAutoHide();
}

void TopLevelWindow::SetMenuBarVisibility(bool visible) {
  window_->SetMenuBarVisibility(visible);
}

bool TopLevelWindow::IsMenuBarVisible() {
  return window_->IsMenuBarVisible();
}

void TopLevelWindow::SetAspectRatio(double aspect_ratio,
                                    mate::Arguments* args) {
  gfx::Size extra_size;
  args->GetNext(&extra_size);
  window_->SetAspectRatio(aspect_ratio, extra_size);
}

void TopLevelWindow::PreviewFile(const std::string& path,
                                 mate::Arguments* args) {
  std::string display_name;
  if (!args->GetNext(&display_name))
    display_name = path;
  window_->PreviewFile(path, display_name);
}

void TopLevelWindow::CloseFilePreview() {
  window_->CloseFilePreview();
}

v8::Local<v8::Value> TopLevelWindow::GetContentView() const {
  if (content_view_.IsEmpty())
    return v8::Null(isolate());
  else
    return v8::Local<v8::Value>::New(isolate(), content_view_);
}

v8::Local<v8::Value> TopLevelWindow::GetParentWindow() const {
  if (parent_window_.IsEmpty())
    return v8::Null(isolate());
  else
    return v8::Local<v8::Value>::New(isolate(), parent_window_);
}

std::vector<v8::Local<v8::Object>> TopLevelWindow::GetChildWindows() const {
  return child_windows_.Values(isolate());
}

v8::Local<v8::Value> TopLevelWindow::GetBrowserView(
    mate::Arguments* args) const {
  if (browser_views_.size() == 0) {
    return v8::Null(isolate());
  } else if (browser_views_.size() == 1) {
    auto first_view = browser_views_.begin();
    return v8::Local<v8::Value>::New(isolate(), (*first_view).second);
  } else {
    args->ThrowError(
        "BrowserWindow have multiple BrowserViews, "
        "Use getBrowserViews() instead");
    return v8::Null(isolate());
  }
}

std::vector<v8::Local<v8::Value>> TopLevelWindow::GetBrowserViews() const {
  std::vector<v8::Local<v8::Value>> ret;

  for (auto const& views_iter : browser_views_) {
    ret.push_back(v8::Local<v8::Value>::New(isolate(), views_iter.second));
  }

  return ret;
}

bool TopLevelWindow::IsModal() const {
  return window_->is_modal();
}

bool TopLevelWindow::SetThumbarButtons(mate::Arguments* args) {
#if defined(OS_WIN)
  std::vector<TaskbarHost::ThumbarButton> buttons;
  if (!args->GetNext(&buttons)) {
    args->ThrowError();
    return false;
  }
  auto* window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbarButtons(
      window_->GetAcceleratedWidget(), buttons);
#else
  return false;
#endif
}

#if defined(TOOLKIT_VIEWS)
void TopLevelWindow::SetIcon(mate::Handle<NativeImage> icon) {
#if defined(OS_WIN)
  static_cast<NativeWindowViews*>(window_.get())
      ->SetIcon(icon->GetHICON(GetSystemMetrics(SM_CXSMICON)),
                icon->GetHICON(GetSystemMetrics(SM_CXICON)));
#elif defined(USE_X11)
  static_cast<NativeWindowViews*>(window_.get())
      ->SetIcon(icon->image().AsImageSkia());
#endif
}
#endif

#if defined(OS_WIN)
bool TopLevelWindow::HookWindowMessage(UINT message,
                                       const MessageCallback& callback) {
  messages_callback_map_[message] = callback;
  return true;
}

void TopLevelWindow::UnhookWindowMessage(UINT message) {
  if (!ContainsKey(messages_callback_map_, message))
    return;

  messages_callback_map_.erase(message);
}

bool TopLevelWindow::IsWindowMessageHooked(UINT message) {
  return ContainsKey(messages_callback_map_, message);
}

void TopLevelWindow::UnhookAllWindowMessages() {
  messages_callback_map_.clear();
}

bool TopLevelWindow::SetThumbnailClip(const gfx::Rect& region) {
  auto* window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbnailClip(
      window_->GetAcceleratedWidget(), region);
}

bool TopLevelWindow::SetThumbnailToolTip(const std::string& tooltip) {
  auto* window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbnailToolTip(
      window_->GetAcceleratedWidget(), tooltip);
}

void TopLevelWindow::SetAppDetails(const mate::Dictionary& options) {
  base::string16 app_id;
  base::FilePath app_icon_path;
  int app_icon_index = 0;
  base::string16 relaunch_command;
  base::string16 relaunch_display_name;

  options.Get("appId", &app_id);
  options.Get("appIconPath", &app_icon_path);
  options.Get("appIconIndex", &app_icon_index);
  options.Get("relaunchCommand", &relaunch_command);
  options.Get("relaunchDisplayName", &relaunch_display_name);

  ui::win::SetAppDetailsForWindow(app_id, app_icon_path, app_icon_index,
                                  relaunch_command, relaunch_display_name,
                                  window_->GetAcceleratedWidget());
}
#endif

int32_t TopLevelWindow::GetID() const {
  return weak_map_id();
}

void TopLevelWindow::ResetBrowserViews() {
  for (auto& item : browser_views_) {
    mate::Handle<BrowserView> browser_view;
    if (mate::ConvertFromV8(isolate(),
                            v8::Local<v8::Value>::New(isolate(), item.second),
                            &browser_view) &&
        !browser_view.IsEmpty()) {
      window_->RemoveBrowserView(browser_view->view());
      browser_view->web_contents()->SetOwnerWindow(nullptr);
    }

    item.second.Reset();
  }

  browser_views_.clear();
}

void TopLevelWindow::RemoveFromParentChildWindows() {
  if (parent_window_.IsEmpty())
    return;

  mate::Handle<TopLevelWindow> parent;
  if (!mate::ConvertFromV8(isolate(), GetParentWindow(), &parent) ||
      parent.IsEmpty()) {
    return;
  }

  parent->child_windows_.Remove(weak_map_id());
}

// static
mate::WrappableBase* TopLevelWindow::New(mate::Arguments* args) {
  mate::Dictionary options;
  if (!(args->Length() == 1 && args->GetNext(&options)))
    options = mate::Dictionary::CreateEmpty(args->isolate());
  return new TopLevelWindow(args->isolate(), args->GetThis(), options);
}

// static
void TopLevelWindow::BuildPrototype(v8::Isolate* isolate,
                                    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "TopLevelWindow"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("setContentView", &TopLevelWindow::SetContentView)
      .SetMethod("close", &TopLevelWindow::Close)
      .SetMethod("focus", &TopLevelWindow::Focus)
      .SetMethod("blur", &TopLevelWindow::Blur)
      .SetMethod("isFocused", &TopLevelWindow::IsFocused)
      .SetMethod("show", &TopLevelWindow::Show)
      .SetMethod("showInactive", &TopLevelWindow::ShowInactive)
      .SetMethod("hide", &TopLevelWindow::Hide)
      .SetMethod("isVisible", &TopLevelWindow::IsVisible)
      .SetMethod("isEnabled", &TopLevelWindow::IsEnabled)
      .SetMethod("setEnabled", &TopLevelWindow::SetEnabled)
      .SetMethod("maximize", &TopLevelWindow::Maximize)
      .SetMethod("unmaximize", &TopLevelWindow::Unmaximize)
      .SetMethod("isMaximized", &TopLevelWindow::IsMaximized)
      .SetMethod("minimize", &TopLevelWindow::Minimize)
      .SetMethod("restore", &TopLevelWindow::Restore)
      .SetMethod("isMinimized", &TopLevelWindow::IsMinimized)
      .SetMethod("setFullScreen", &TopLevelWindow::SetFullScreen)
      .SetMethod("isFullScreen", &TopLevelWindow::IsFullscreen)
      .SetMethod("setBounds", &TopLevelWindow::SetBounds)
      .SetMethod("getBounds", &TopLevelWindow::GetBounds)
      .SetMethod("isNormal", &TopLevelWindow::IsNormal)
      .SetMethod("getNormalBounds", &TopLevelWindow::GetNormalBounds)
      .SetMethod("setSize", &TopLevelWindow::SetSize)
      .SetMethod("getSize", &TopLevelWindow::GetSize)
      .SetMethod("setContentBounds", &TopLevelWindow::SetContentBounds)
      .SetMethod("getContentBounds", &TopLevelWindow::GetContentBounds)
      .SetMethod("setContentSize", &TopLevelWindow::SetContentSize)
      .SetMethod("getContentSize", &TopLevelWindow::GetContentSize)
      .SetMethod("setMinimumSize", &TopLevelWindow::SetMinimumSize)
      .SetMethod("getMinimumSize", &TopLevelWindow::GetMinimumSize)
      .SetMethod("setMaximumSize", &TopLevelWindow::SetMaximumSize)
      .SetMethod("getMaximumSize", &TopLevelWindow::GetMaximumSize)
      .SetMethod("setSheetOffset", &TopLevelWindow::SetSheetOffset)
      .SetMethod("setResizable", &TopLevelWindow::SetResizable)
      .SetMethod("isResizable", &TopLevelWindow::IsResizable)
      .SetMethod("setMovable", &TopLevelWindow::SetMovable)
#if defined(OS_WIN) || defined(OS_MACOSX)
      .SetMethod("moveTop", &TopLevelWindow::MoveTop)
#endif
      .SetMethod("isMovable", &TopLevelWindow::IsMovable)
      .SetMethod("setMinimizable", &TopLevelWindow::SetMinimizable)
      .SetMethod("isMinimizable", &TopLevelWindow::IsMinimizable)
      .SetMethod("setMaximizable", &TopLevelWindow::SetMaximizable)
      .SetMethod("isMaximizable", &TopLevelWindow::IsMaximizable)
      .SetMethod("setFullScreenable", &TopLevelWindow::SetFullScreenable)
      .SetMethod("isFullScreenable", &TopLevelWindow::IsFullScreenable)
      .SetMethod("setClosable", &TopLevelWindow::SetClosable)
      .SetMethod("isClosable", &TopLevelWindow::IsClosable)
      .SetMethod("setAlwaysOnTop", &TopLevelWindow::SetAlwaysOnTop)
      .SetMethod("isAlwaysOnTop", &TopLevelWindow::IsAlwaysOnTop)
      .SetMethod("center", &TopLevelWindow::Center)
      .SetMethod("setPosition", &TopLevelWindow::SetPosition)
      .SetMethod("getPosition", &TopLevelWindow::GetPosition)
      .SetMethod("setTitle", &TopLevelWindow::SetTitle)
      .SetMethod("getTitle", &TopLevelWindow::GetTitle)
      .SetMethod("flashFrame", &TopLevelWindow::FlashFrame)
      .SetMethod("setSkipTaskbar", &TopLevelWindow::SetSkipTaskbar)
      .SetMethod("setSimpleFullScreen", &TopLevelWindow::SetSimpleFullScreen)
      .SetMethod("isSimpleFullScreen", &TopLevelWindow::IsSimpleFullScreen)
      .SetMethod("setKiosk", &TopLevelWindow::SetKiosk)
      .SetMethod("isKiosk", &TopLevelWindow::IsKiosk)
      .SetMethod("setBackgroundColor", &TopLevelWindow::SetBackgroundColor)
      .SetMethod("setHasShadow", &TopLevelWindow::SetHasShadow)
      .SetMethod("hasShadow", &TopLevelWindow::HasShadow)
      .SetMethod("setOpacity", &TopLevelWindow::SetOpacity)
      .SetMethod("getOpacity", &TopLevelWindow::GetOpacity)
      .SetMethod("setShape", &TopLevelWindow::SetShape)
      .SetMethod("setRepresentedFilename",
                 &TopLevelWindow::SetRepresentedFilename)
      .SetMethod("getRepresentedFilename",
                 &TopLevelWindow::GetRepresentedFilename)
      .SetMethod("setDocumentEdited", &TopLevelWindow::SetDocumentEdited)
      .SetMethod("isDocumentEdited", &TopLevelWindow::IsDocumentEdited)
      .SetMethod("setIgnoreMouseEvents", &TopLevelWindow::SetIgnoreMouseEvents)
      .SetMethod("setContentProtection", &TopLevelWindow::SetContentProtection)
      .SetMethod("setFocusable", &TopLevelWindow::SetFocusable)
      .SetMethod("setMenu", &TopLevelWindow::SetMenu)
      .SetMethod("removeMenu", &TopLevelWindow::RemoveMenu)
      .SetMethod("setParentWindow", &TopLevelWindow::SetParentWindow)
      .SetMethod("setBrowserView", &TopLevelWindow::SetBrowserView)
      .SetMethod("addBrowserView", &TopLevelWindow::AddBrowserView)
      .SetMethod("removeBrowserView", &TopLevelWindow::RemoveBrowserView)
      .SetMethod("getNativeWindowHandle",
                 &TopLevelWindow::GetNativeWindowHandle)
      .SetMethod("setProgressBar", &TopLevelWindow::SetProgressBar)
      .SetMethod("setOverlayIcon", &TopLevelWindow::SetOverlayIcon)
      .SetMethod("setVisibleOnAllWorkspaces",
                 &TopLevelWindow::SetVisibleOnAllWorkspaces)
      .SetMethod("isVisibleOnAllWorkspaces",
                 &TopLevelWindow::IsVisibleOnAllWorkspaces)
#if defined(OS_MACOSX)
      .SetMethod("setAutoHideCursor", &TopLevelWindow::SetAutoHideCursor)
#endif
      .SetMethod("setVibrancy", &TopLevelWindow::SetVibrancy)
      .SetMethod("_setTouchBarItems", &TopLevelWindow::SetTouchBar)
      .SetMethod("_refreshTouchBarItem", &TopLevelWindow::RefreshTouchBarItem)
      .SetMethod("_setEscapeTouchBarItem",
                 &TopLevelWindow::SetEscapeTouchBarItem)
#if defined(OS_MACOSX)
      .SetMethod("selectPreviousTab", &TopLevelWindow::SelectPreviousTab)
      .SetMethod("selectNextTab", &TopLevelWindow::SelectNextTab)
      .SetMethod("mergeAllWindows", &TopLevelWindow::MergeAllWindows)
      .SetMethod("moveTabToNewWindow", &TopLevelWindow::MoveTabToNewWindow)
      .SetMethod("toggleTabBar", &TopLevelWindow::ToggleTabBar)
      .SetMethod("addTabbedWindow", &TopLevelWindow::AddTabbedWindow)
      .SetMethod("setWindowButtonVisibility",
                 &TopLevelWindow::SetWindowButtonVisibility)
#endif
      .SetMethod("setAutoHideMenuBar", &TopLevelWindow::SetAutoHideMenuBar)
      .SetMethod("isMenuBarAutoHide", &TopLevelWindow::IsMenuBarAutoHide)
      .SetMethod("setMenuBarVisibility", &TopLevelWindow::SetMenuBarVisibility)
      .SetMethod("isMenuBarVisible", &TopLevelWindow::IsMenuBarVisible)
      .SetMethod("setAspectRatio", &TopLevelWindow::SetAspectRatio)
      .SetMethod("previewFile", &TopLevelWindow::PreviewFile)
      .SetMethod("closeFilePreview", &TopLevelWindow::CloseFilePreview)
      .SetMethod("getContentView", &TopLevelWindow::GetContentView)
      .SetMethod("getParentWindow", &TopLevelWindow::GetParentWindow)
      .SetMethod("getChildWindows", &TopLevelWindow::GetChildWindows)
      .SetMethod("getBrowserView", &TopLevelWindow::GetBrowserView)
      .SetMethod("getBrowserViews", &TopLevelWindow::GetBrowserViews)
      .SetMethod("isModal", &TopLevelWindow::IsModal)
      .SetMethod("setThumbarButtons", &TopLevelWindow::SetThumbarButtons)
#if defined(TOOLKIT_VIEWS)
      .SetMethod("setIcon", &TopLevelWindow::SetIcon)
#endif
#if defined(OS_WIN)
      .SetMethod("hookWindowMessage", &TopLevelWindow::HookWindowMessage)
      .SetMethod("isWindowMessageHooked",
                 &TopLevelWindow::IsWindowMessageHooked)
      .SetMethod("unhookWindowMessage", &TopLevelWindow::UnhookWindowMessage)
      .SetMethod("unhookAllWindowMessages",
                 &TopLevelWindow::UnhookAllWindowMessages)
      .SetMethod("setThumbnailClip", &TopLevelWindow::SetThumbnailClip)
      .SetMethod("setThumbnailToolTip", &TopLevelWindow::SetThumbnailToolTip)
      .SetMethod("setAppDetails", &TopLevelWindow::SetAppDetails)
#endif
      .SetProperty("id", &TopLevelWindow::GetID);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::TopLevelWindow;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  TopLevelWindow::SetConstructor(isolate, base::Bind(&TopLevelWindow::New));

  mate::Dictionary constructor(isolate, TopLevelWindow::GetConstructor(isolate)
                                            ->GetFunction(context)
                                            .ToLocalChecked());
  constructor.SetMethod("fromId",
                        &mate::TrackableObject<TopLevelWindow>::FromWeakMapID);
  constructor.SetMethod("getAllWindows",
                        &mate::TrackableObject<TopLevelWindow>::GetAll);

  mate::Dictionary dict(isolate, exports);
  dict.Set("TopLevelWindow", constructor);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_top_level_window, Initialize)

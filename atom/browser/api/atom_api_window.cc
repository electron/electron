// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_window.h"
#include "atom/common/native_mate_converters/value_converter.h"

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/geometry/rect.h"

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

template<>
struct Converter<atom::TaskbarHost::ThumbarButton> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
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


Window::Window(v8::Isolate* isolate, v8::Local<v8::Object> wrapper,
               const mate::Dictionary& options) {
  mate::Handle<class WebContents> web_contents;
  // If no WebContents was passed to the constructor, create it from options.
  if (!options.Get("webContents", &web_contents)) {
    // Use options.webPreferences to create WebContents.
    mate::Dictionary web_preferences = mate::Dictionary::CreateEmpty(isolate);
    options.Get(options::kWebPreferences, &web_preferences);

    // Copy the backgroundColor to webContents.
    v8::Local<v8::Value> value;
    if (options.Get(options::kBackgroundColor, &value))
      web_preferences.Set(options::kBackgroundColor, value);

    v8::Local<v8::Value> transparent;
    if (options.Get("transparent", &transparent))
      web_preferences.Set("transparent", transparent);

    // Offscreen windows are always created frameless.
    bool offscreen;
    if (web_preferences.Get("offscreen", &offscreen) && offscreen) {
      auto window_options = const_cast<mate::Dictionary&>(options);
      window_options.Set(options::kFrame, false);
    }

    // Creates the WebContents used by BrowserWindow.
    web_contents = WebContents::Create(isolate, web_preferences);
  }

  Init(isolate, wrapper, options, web_contents);
}

void Window::Init(v8::Isolate* isolate,
                  v8::Local<v8::Object> wrapper,
                  const mate::Dictionary& options,
                  mate::Handle<class WebContents> web_contents) {
  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents.get();

  // Keep a copy of the options for later use.
  mate::Dictionary(isolate, web_contents->GetWrapper()).Set(
      "browserWindowOptions", options);

  // The parent window.
  mate::Handle<Window> parent;
  if (options.Get("parent", &parent))
    parent_window_.Reset(isolate, parent.ToV8());

  // Creates BrowserWindow.
  window_.reset(NativeWindow::Create(
      web_contents->managed_web_contents(),
      options,
      parent.IsEmpty() ? nullptr : parent->window_.get()));
  web_contents->SetOwnerWindow(window_.get());

#if defined(TOOLKIT_VIEWS)
  // Sets the window icon.
  mate::Handle<NativeImage> icon;
  if (options.Get(options::kIcon, &icon))
    SetIcon(icon);
#endif

  window_->InitFromOptions(options);
  window_->AddObserver(this);

  InitWith(isolate, wrapper);
  AttachAsUserData(window_.get());

  // We can only append this window to parent window's child windows after this
  // window's JS wrapper gets initialized.
  if (!parent.IsEmpty())
    parent->child_windows_.Set(isolate, ID(), wrapper);
}

Window::~Window() {
  if (!window_->IsClosed())
    window_->CloseContents(nullptr);

  // Destroy the native window in next tick because the native code might be
  // iterating all windows.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, window_.release());
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::WillDestroyNativeObject() {
  // Close all child windows before closing current window.
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  for (v8::Local<v8::Value> value : child_windows_.Values(isolate())) {
    mate::Handle<Window> child;
    if (mate::ConvertFromV8(isolate(), value, &child))
      child->window_->CloseImmediately();
  }
}

void Window::OnWindowClosed() {
  api_web_contents_->DestroyWebContents();

  RemoveFromWeakMap();
  window_->RemoveObserver(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  Emit("closed");

  RemoveFromParentChildWindows();

  // Destroy the native class when window is closed.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, GetDestroyClosure());
}

void Window::OnWindowBlur() {
  Emit("blur");
}

void Window::OnWindowFocus() {
  Emit("focus");
}

void Window::OnWindowShow() {
  Emit("show");
}

void Window::OnWindowHide() {
  Emit("hide");
}

void Window::OnReadyToShow() {
  Emit("ready-to-show");
}

void Window::OnWindowMaximize() {
  Emit("maximize");
}

void Window::OnWindowUnmaximize() {
  Emit("unmaximize");
}

void Window::OnWindowMinimize() {
  Emit("minimize");
}

void Window::OnWindowRestore() {
  Emit("restore");
}

void Window::OnWindowResize() {
  Emit("resize");
}

void Window::OnWindowMove() {
  Emit("move");
}

void Window::OnWindowMoved() {
  Emit("moved");
}

void Window::OnWindowEnterFullScreen() {
  Emit("enter-full-screen");
}

void Window::OnWindowLeaveFullScreen() {
  Emit("leave-full-screen");
}

void Window::OnWindowScrollTouchBegin() {
  Emit("scroll-touch-begin");
}

void Window::OnWindowScrollTouchEnd() {
  Emit("scroll-touch-end");
}

void Window::OnWindowScrollTouchEdge() {
  Emit("scroll-touch-edge");
}

void Window::OnWindowSwipe(const std::string& direction) {
  Emit("swipe", direction);
}

void Window::OnWindowEnterHtmlFullScreen() {
  Emit("enter-html-full-screen");
}

void Window::OnWindowLeaveHtmlFullScreen() {
  Emit("leave-html-full-screen");
}

void Window::OnRendererUnresponsive() {
  Emit("unresponsive");
}

void Window::OnRendererResponsive() {
  Emit("responsive");
}

void Window::OnExecuteWindowsCommand(const std::string& command_name) {
  Emit("app-command", command_name);
}

void Window::OnTouchBarItemResult(const std::string& item_id,
                                  const base::DictionaryValue& details) {
  Emit("-touch-bar-interaction", item_id, details);
}

#if defined(OS_WIN)
void Window::OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {
  if (IsWindowMessageHooked(message)) {
    messages_callback_map_[message].Run(
        ToBuffer(isolate(), static_cast<void*>(&w_param), sizeof(WPARAM)),
        ToBuffer(isolate(), static_cast<void*>(&l_param), sizeof(LPARAM)));
  }
}
#endif

// static
mate::WrappableBase* Window::New(mate::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    args->ThrowError("Cannot create BrowserWindow before app is ready");
    return nullptr;
  }

  if (args->Length() > 1) {
    args->ThrowError();
    return nullptr;
  }

  mate::Dictionary options;
  if (!(args->Length() == 1 && args->GetNext(&options))) {
    options = mate::Dictionary::CreateEmpty(args->isolate());
  }

  return new Window(args->isolate(), args->GetThis(), options);
}

void Window::Close() {
  window_->Close();
}

void Window::Focus() {
  window_->Focus(true);
}

void Window::Blur() {
  window_->Focus(false);
}

bool Window::IsFocused() {
  return window_->IsFocused();
}

void Window::Show() {
  window_->Show();
}

void Window::ShowInactive() {
  // This method doesn't make sense for modal window..
  if (IsModal())
    return;

  window_->ShowInactive();
}

void Window::Hide() {
  window_->Hide();
}

bool Window::IsVisible() {
  return window_->IsVisible();
}

bool Window::IsEnabled() {
  return window_->IsEnabled();
}

void Window::Maximize() {
  window_->Maximize();
}

void Window::Unmaximize() {
  window_->Unmaximize();
}

bool Window::IsMaximized() {
  return window_->IsMaximized();
}

void Window::Minimize() {
  window_->Minimize();
}

void Window::Restore() {
  window_->Restore();
}

bool Window::IsMinimized() {
  return window_->IsMinimized();
}

void Window::SetFullScreen(bool fullscreen) {
  window_->SetFullScreen(fullscreen);
}

bool Window::IsFullscreen() {
  return window_->IsFullscreen();
}

void Window::SetBounds(const gfx::Rect& bounds, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetBounds(bounds, animate);
}

gfx::Rect Window::GetBounds() {
  return window_->GetBounds();
}

void Window::SetContentBounds(const gfx::Rect& bounds, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentBounds(bounds, animate);
}

gfx::Rect Window::GetContentBounds() {
  return window_->GetContentBounds();
}

void Window::SetSize(int width, int height, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetSize(gfx::Size(width, height), animate);
}

std::vector<int> Window::GetSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetContentSize(int width, int height, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetContentSize(gfx::Size(width, height), animate);
}

std::vector<int> Window::GetContentSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetContentSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetMinimumSize(int width, int height) {
  window_->SetMinimumSize(gfx::Size(width, height));
}

std::vector<int> Window::GetMinimumSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetMinimumSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetMaximumSize(int width, int height) {
  window_->SetMaximumSize(gfx::Size(width, height));
}

std::vector<int> Window::GetMaximumSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetMaximumSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetSheetOffset(double offsetY, mate::Arguments* args) {
  double offsetX = 0.0;
  args->GetNext(&offsetX);
  window_->SetSheetOffset(offsetX, offsetY);
}

void Window::SetResizable(bool resizable) {
  window_->SetResizable(resizable);
}

bool Window::IsResizable() {
  return window_->IsResizable();
}

void Window::SetMovable(bool movable) {
  window_->SetMovable(movable);
}

bool Window::IsMovable() {
  return window_->IsMovable();
}

void Window::SetMinimizable(bool minimizable) {
  window_->SetMinimizable(minimizable);
}

bool Window::IsMinimizable() {
  return window_->IsMinimizable();
}

void Window::SetMaximizable(bool maximizable) {
  window_->SetMaximizable(maximizable);
}

bool Window::IsMaximizable() {
  return window_->IsMaximizable();
}

void Window::SetFullScreenable(bool fullscreenable) {
  window_->SetFullScreenable(fullscreenable);
}

bool Window::IsFullScreenable() {
  return window_->IsFullScreenable();
}

void Window::SetClosable(bool closable) {
  window_->SetClosable(closable);
}

bool Window::IsClosable() {
  return window_->IsClosable();
}

void Window::SetAlwaysOnTop(bool top, mate::Arguments* args) {
  std::string level = "floating";
  int relativeLevel = 0;
  std::string error;

  args->GetNext(&level);
  args->GetNext(&relativeLevel);

  window_->SetAlwaysOnTop(top, level, relativeLevel, &error);

  if (!error.empty()) {
    args->ThrowError(error);
  }
}

bool Window::IsAlwaysOnTop() {
  return window_->IsAlwaysOnTop();
}

void Window::Center() {
  window_->Center();
}

void Window::SetPosition(int x, int y, mate::Arguments* args) {
  bool animate = false;
  args->GetNext(&animate);
  window_->SetPosition(gfx::Point(x, y), animate);
}

std::vector<int> Window::GetPosition() {
  std::vector<int> result(2);
  gfx::Point pos = window_->GetPosition();
  result[0] = pos.x();
  result[1] = pos.y();
  return result;
}

void Window::SetTitle(const std::string& title) {
  window_->SetTitle(title);
}

std::string Window::GetTitle() {
  return window_->GetTitle();
}

void Window::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

void Window::SetSkipTaskbar(bool skip) {
  window_->SetSkipTaskbar(skip);
}

void Window::SetKiosk(bool kiosk) {
  window_->SetKiosk(kiosk);
}

bool Window::IsKiosk() {
  return window_->IsKiosk();
}

void Window::SetBackgroundColor(const std::string& color_name) {
  window_->SetBackgroundColor(color_name);
}

void Window::SetHasShadow(bool has_shadow) {
  window_->SetHasShadow(has_shadow);
}

bool Window::HasShadow() {
  return window_->HasShadow();
}

void Window::FocusOnWebView() {
  window_->FocusOnWebView();
}

void Window::BlurWebView() {
  window_->BlurWebView();
}

bool Window::IsWebViewFocused() {
  return window_->IsWebViewFocused();
}

void Window::SetRepresentedFilename(const std::string& filename) {
  window_->SetRepresentedFilename(filename);
}

std::string Window::GetRepresentedFilename() {
  return window_->GetRepresentedFilename();
}

void Window::SetDocumentEdited(bool edited) {
  window_->SetDocumentEdited(edited);
}

bool Window::IsDocumentEdited() {
  return window_->IsDocumentEdited();
}

void Window::SetIgnoreMouseEvents(bool ignore) {
  return window_->SetIgnoreMouseEvents(ignore);
}

void Window::SetContentProtection(bool enable) {
  return window_->SetContentProtection(enable);
}

void Window::SetFocusable(bool focusable) {
  return window_->SetFocusable(focusable);
}

void Window::SetProgressBar(double progress, mate::Arguments* args) {
  mate::Dictionary options;
  std::string mode;
  NativeWindow::ProgressState state = NativeWindow::PROGRESS_NORMAL;

  args->GetNext(&options) && options.Get("mode", &mode);

  if (mode == "error") {
    state = NativeWindow::PROGRESS_ERROR;
  } else if (mode == "paused") {
    state = NativeWindow::PROGRESS_PAUSED;
  } else if (mode == "indeterminate") {
    state = NativeWindow::PROGRESS_INDETERMINATE;
  } else if (mode == "none") {
    state = NativeWindow::PROGRESS_NONE;
  }

  window_->SetProgressBar(progress, state);
}

void Window::SetOverlayIcon(const gfx::Image& overlay,
                            const std::string& description) {
  window_->SetOverlayIcon(overlay, description);
}

bool Window::SetThumbarButtons(mate::Arguments* args) {
#if defined(OS_WIN)
  std::vector<TaskbarHost::ThumbarButton> buttons;
  if (!args->GetNext(&buttons)) {
    args->ThrowError();
    return false;
  }
  auto window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbarButtons(
      window_->GetAcceleratedWidget(), buttons);
#else
  return false;
#endif
}

void Window::SetMenu(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  mate::Handle<Menu> menu;
  if (value->IsObject() &&
      mate::V8ToString(value->ToObject()->GetConstructorName()) == "Menu" &&
      mate::ConvertFromV8(isolate, value, &menu)) {
    menu_.Reset(isolate, menu.ToV8());
    window_->SetMenu(menu->model());
  } else if (value->IsNull()) {
    menu_.Reset();
    window_->SetMenu(nullptr);
  } else {
    isolate->ThrowException(v8::Exception::TypeError(
        mate::StringToV8(isolate, "Invalid Menu")));
  }
}

void Window::SetAutoHideMenuBar(bool auto_hide) {
  window_->SetAutoHideMenuBar(auto_hide);
}

bool Window::IsMenuBarAutoHide() {
  return window_->IsMenuBarAutoHide();
}

void Window::SetMenuBarVisibility(bool visible) {
  window_->SetMenuBarVisibility(visible);
}

bool Window::IsMenuBarVisible() {
  return window_->IsMenuBarVisible();
}

#if defined(OS_WIN)
bool Window::HookWindowMessage(UINT message,
                               const MessageCallback& callback) {
  messages_callback_map_[message] = callback;
  return true;
}

void Window::UnhookWindowMessage(UINT message) {
  if (!ContainsKey(messages_callback_map_, message))
    return;

  messages_callback_map_.erase(message);
}

bool Window::IsWindowMessageHooked(UINT message) {
  return ContainsKey(messages_callback_map_, message);
}

void Window::UnhookAllWindowMessages() {
  messages_callback_map_.clear();
}

bool Window::SetThumbnailClip(const gfx::Rect& region) {
  auto window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbnailClip(
      window_->GetAcceleratedWidget(), region);
}

bool Window::SetThumbnailToolTip(const std::string& tooltip) {
  auto window = static_cast<NativeWindowViews*>(window_.get());
  return window->taskbar_host().SetThumbnailToolTip(
      window_->GetAcceleratedWidget(), tooltip);
}

void Window::SetAppDetails(const mate::Dictionary& options) {
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

  ui::win::SetAppDetailsForWindow(
      app_id, app_icon_path, app_icon_index,
      relaunch_command, relaunch_display_name,
      window_->GetAcceleratedWidget());
}
#endif

#if defined(TOOLKIT_VIEWS)
void Window::SetIcon(mate::Handle<NativeImage> icon) {
#if defined(OS_WIN)
  static_cast<NativeWindowViews*>(window_.get())->SetIcon(
      icon->GetHICON(GetSystemMetrics(SM_CXSMICON)),
      icon->GetHICON(GetSystemMetrics(SM_CXICON)));
#elif defined(USE_X11)
  static_cast<NativeWindowViews*>(window_.get())->SetIcon(
      icon->image().AsImageSkia());
#endif
}
#endif

void Window::SetAspectRatio(double aspect_ratio, mate::Arguments* args) {
  gfx::Size extra_size;
  args->GetNext(&extra_size);
  window_->SetAspectRatio(aspect_ratio, extra_size);
}

void Window::PreviewFile(const std::string& path, mate::Arguments* args) {
  std::string display_name;
  if (!args->GetNext(&display_name))
    display_name = path;
  window_->PreviewFile(path, display_name);
}

void Window::CloseFilePreview() {
  window_->CloseFilePreview();
}

void Window::SetParentWindow(v8::Local<v8::Value> value,
                             mate::Arguments* args) {
  if (IsModal()) {
    args->ThrowError("Can not be called for modal window");
    return;
  }

  mate::Handle<Window> parent;
  if (value->IsNull()) {
    RemoveFromParentChildWindows();
    parent_window_.Reset();
    window_->SetParentWindow(nullptr);
  } else if (mate::ConvertFromV8(isolate(), value, &parent)) {
    parent_window_.Reset(isolate(), value);
    window_->SetParentWindow(parent->window_.get());
    parent->child_windows_.Set(isolate(), ID(), GetWrapper());
  } else {
    args->ThrowError("Must pass BrowserWindow instance or null");
  }
}

v8::Local<v8::Value> Window::GetParentWindow() const {
  if (parent_window_.IsEmpty())
    return v8::Null(isolate());
  else
    return v8::Local<v8::Value>::New(isolate(), parent_window_);
}

std::vector<v8::Local<v8::Object>> Window::GetChildWindows() const {
  return child_windows_.Values(isolate());
}

bool Window::IsModal() const {
  return window_->is_modal();
}

v8::Local<v8::Value> Window::GetNativeWindowHandle() {
  gfx::AcceleratedWidget handle = window_->GetAcceleratedWidget();
  return ToBuffer(
      isolate(), static_cast<void*>(&handle), sizeof(gfx::AcceleratedWidget));
}

void Window::SetVisibleOnAllWorkspaces(bool visible) {
  return window_->SetVisibleOnAllWorkspaces(visible);
}

bool Window::IsVisibleOnAllWorkspaces() {
  return window_->IsVisibleOnAllWorkspaces();
}

void Window::SetAutoHideCursor(bool auto_hide) {
  window_->SetAutoHideCursor(auto_hide);
}

void Window::SetVibrancy(mate::Arguments* args) {
  std::string type;

  args->GetNext(&type);
  window_->SetVibrancy(type);
}

void Window::SetTouchBar(const std::vector<mate::PersistentDictionary>& items) {
  window_->SetTouchBar(items);
}

void Window::RefreshTouchBarItem(const std::string& item_id) {
  window_->RefreshTouchBarItem(item_id);
}

int32_t Window::ID() const {
  return weak_map_id();
}

v8::Local<v8::Value> Window::WebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, web_contents_);
}

void Window::RemoveFromParentChildWindows() {
  if (parent_window_.IsEmpty())
    return;

  mate::Handle<Window> parent;
  if (!mate::ConvertFromV8(isolate(), GetParentWindow(), &parent))
    return;

  parent->child_windows_.Remove(ID());
}

// static
void Window::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "BrowserWindow"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("close", &Window::Close)
      .SetMethod("focus", &Window::Focus)
      .SetMethod("blur", &Window::Blur)
      .SetMethod("isFocused", &Window::IsFocused)
      .SetMethod("show", &Window::Show)
      .SetMethod("showInactive", &Window::ShowInactive)
      .SetMethod("hide", &Window::Hide)
      .SetMethod("isVisible", &Window::IsVisible)
      .SetMethod("isEnabled", &Window::IsEnabled)
      .SetMethod("maximize", &Window::Maximize)
      .SetMethod("unmaximize", &Window::Unmaximize)
      .SetMethod("isMaximized", &Window::IsMaximized)
      .SetMethod("minimize", &Window::Minimize)
      .SetMethod("restore", &Window::Restore)
      .SetMethod("isMinimized", &Window::IsMinimized)
      .SetMethod("setFullScreen", &Window::SetFullScreen)
      .SetMethod("isFullScreen", &Window::IsFullscreen)
      .SetMethod("setAspectRatio", &Window::SetAspectRatio)
      .SetMethod("previewFile", &Window::PreviewFile)
      .SetMethod("closeFilePreview", &Window::CloseFilePreview)
#if !defined(OS_WIN)
      .SetMethod("setParentWindow", &Window::SetParentWindow)
#endif
      .SetMethod("getParentWindow", &Window::GetParentWindow)
      .SetMethod("getChildWindows", &Window::GetChildWindows)
      .SetMethod("isModal", &Window::IsModal)
      .SetMethod("getNativeWindowHandle", &Window::GetNativeWindowHandle)
      .SetMethod("getBounds", &Window::GetBounds)
      .SetMethod("setBounds", &Window::SetBounds)
      .SetMethod("getSize", &Window::GetSize)
      .SetMethod("setSize", &Window::SetSize)
      .SetMethod("getContentBounds", &Window::GetContentBounds)
      .SetMethod("setContentBounds", &Window::SetContentBounds)
      .SetMethod("getContentSize", &Window::GetContentSize)
      .SetMethod("setContentSize", &Window::SetContentSize)
      .SetMethod("setMinimumSize", &Window::SetMinimumSize)
      .SetMethod("getMinimumSize", &Window::GetMinimumSize)
      .SetMethod("setMaximumSize", &Window::SetMaximumSize)
      .SetMethod("getMaximumSize", &Window::GetMaximumSize)
      .SetMethod("setSheetOffset", &Window::SetSheetOffset)
      .SetMethod("setResizable", &Window::SetResizable)
      .SetMethod("isResizable", &Window::IsResizable)
      .SetMethod("setMovable", &Window::SetMovable)
      .SetMethod("isMovable", &Window::IsMovable)
      .SetMethod("setMinimizable", &Window::SetMinimizable)
      .SetMethod("isMinimizable", &Window::IsMinimizable)
      .SetMethod("setMaximizable", &Window::SetMaximizable)
      .SetMethod("isMaximizable", &Window::IsMaximizable)
      .SetMethod("setFullScreenable", &Window::SetFullScreenable)
      .SetMethod("isFullScreenable", &Window::IsFullScreenable)
      .SetMethod("setClosable", &Window::SetClosable)
      .SetMethod("isClosable", &Window::IsClosable)
      .SetMethod("setAlwaysOnTop", &Window::SetAlwaysOnTop)
      .SetMethod("isAlwaysOnTop", &Window::IsAlwaysOnTop)
      .SetMethod("center", &Window::Center)
      .SetMethod("setPosition", &Window::SetPosition)
      .SetMethod("getPosition", &Window::GetPosition)
      .SetMethod("setTitle", &Window::SetTitle)
      .SetMethod("getTitle", &Window::GetTitle)
      .SetMethod("flashFrame", &Window::FlashFrame)
      .SetMethod("setSkipTaskbar", &Window::SetSkipTaskbar)
      .SetMethod("setKiosk", &Window::SetKiosk)
      .SetMethod("isKiosk", &Window::IsKiosk)
      .SetMethod("setBackgroundColor", &Window::SetBackgroundColor)
      .SetMethod("setHasShadow", &Window::SetHasShadow)
      .SetMethod("hasShadow", &Window::HasShadow)
      .SetMethod("setRepresentedFilename", &Window::SetRepresentedFilename)
      .SetMethod("getRepresentedFilename", &Window::GetRepresentedFilename)
      .SetMethod("setDocumentEdited", &Window::SetDocumentEdited)
      .SetMethod("isDocumentEdited", &Window::IsDocumentEdited)
      .SetMethod("setIgnoreMouseEvents", &Window::SetIgnoreMouseEvents)
      .SetMethod("setContentProtection", &Window::SetContentProtection)
      .SetMethod("setFocusable", &Window::SetFocusable)
      .SetMethod("focusOnWebView", &Window::FocusOnWebView)
      .SetMethod("blurWebView", &Window::BlurWebView)
      .SetMethod("isWebViewFocused", &Window::IsWebViewFocused)
      .SetMethod("setProgressBar", &Window::SetProgressBar)
      .SetMethod("setOverlayIcon", &Window::SetOverlayIcon)
      .SetMethod("setThumbarButtons", &Window::SetThumbarButtons)
      .SetMethod("setMenu", &Window::SetMenu)
      .SetMethod("setAutoHideMenuBar", &Window::SetAutoHideMenuBar)
      .SetMethod("isMenuBarAutoHide", &Window::IsMenuBarAutoHide)
      .SetMethod("setMenuBarVisibility", &Window::SetMenuBarVisibility)
      .SetMethod("isMenuBarVisible", &Window::IsMenuBarVisible)
      .SetMethod("setVisibleOnAllWorkspaces",
                 &Window::SetVisibleOnAllWorkspaces)
      .SetMethod("isVisibleOnAllWorkspaces",
                 &Window::IsVisibleOnAllWorkspaces)
#if defined(OS_MACOSX)
      .SetMethod("setAutoHideCursor", &Window::SetAutoHideCursor)
#endif
      .SetMethod("setVibrancy", &Window::SetVibrancy)
      .SetMethod("_setTouchBarItems", &Window::SetTouchBar)
      .SetMethod("_refreshTouchBarItem", &Window::RefreshTouchBarItem)
#if defined(OS_WIN)
      .SetMethod("hookWindowMessage", &Window::HookWindowMessage)
      .SetMethod("isWindowMessageHooked", &Window::IsWindowMessageHooked)
      .SetMethod("unhookWindowMessage", &Window::UnhookWindowMessage)
      .SetMethod("unhookAllWindowMessages", &Window::UnhookAllWindowMessages)
      .SetMethod("setThumbnailClip", &Window::SetThumbnailClip)
      .SetMethod("setThumbnailToolTip", &Window::SetThumbnailToolTip)
      .SetMethod("setAppDetails", &Window::SetAppDetails)
#endif
#if defined(TOOLKIT_VIEWS)
      .SetMethod("setIcon", &Window::SetIcon)
#endif
      .SetProperty("id", &Window::ID)
      .SetProperty("webContents", &Window::WebContents);
}

// static
v8::Local<v8::Value> Window::From(v8::Isolate* isolate,
                                  NativeWindow* native_window) {
  auto existing = TrackableObject::FromWrappedClass(isolate, native_window);
  if (existing)
    return existing->GetWrapper();
  else
    return v8::Null(isolate);
}

}  // namespace api

}  // namespace atom


namespace {

using atom::api::Window;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  Window::SetConstructor(isolate, base::Bind(&Window::New));

  mate::Dictionary browser_window(
      isolate, Window::GetConstructor(isolate)->GetFunction());
  browser_window.SetMethod("fromId",
                           &mate::TrackableObject<Window>::FromWeakMapID);
  browser_window.SetMethod("getAllWindows",
                           &mate::TrackableObject<Window>::GetAll);

  mate::Dictionary dict(isolate, exports);
  dict.Set("BrowserWindow", browser_window);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_window, Initialize)

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
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/geometry/rect.h"

#if defined(OS_WIN)
#include "atom/browser/native_window_views.h"
#include "atom/browser/ui/win/taskbar_host.h"
#endif

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

void OnCapturePageDone(
    v8::Isolate* isolate,
    const base::Callback<void(const gfx::Image&)>& callback,
    const SkBitmap& bitmap) {
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  callback.Run(gfx::Image::CreateFrom1xBitmap(bitmap));
}

// Converts binary data to Buffer.
v8::Local<v8::Value> ToBuffer(v8::Isolate* isolate, void* val, int size) {
  auto buffer = node::Buffer::Copy(isolate, static_cast<char*>(val), size);
  if (buffer.IsEmpty())
    return v8::Null(isolate);
  else
    return buffer.ToLocalChecked();
}

}  // namespace


Window::Window(v8::Isolate* isolate, const mate::Dictionary& options) {
  // Use options.webPreferences to create WebContents.
  mate::Dictionary web_preferences = mate::Dictionary::CreateEmpty(isolate);
  options.Get(options::kWebPreferences, &web_preferences);

  // Copy the backgroundColor to webContents.
  v8::Local<v8::Value> value;
  if (options.Get(options::kBackgroundColor, &value))
    web_preferences.Set(options::kBackgroundColor, value);

  // Creates the WebContents used by BrowserWindow.
  auto web_contents = WebContents::Create(isolate, web_preferences);
  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents.get();

  // Keep a copy of the options for later use.
  mate::Dictionary(isolate, web_contents->GetWrapper()).Set(
      "browserWindowOptions", options);

  // Creates BrowserWindow.
  window_.reset(NativeWindow::Create(web_contents->managed_web_contents(),
                                     options));
  web_contents->SetOwnerWindow(window_.get());
  window_->InitFromOptions(options);
  window_->AddObserver(this);
  AttachAsUserData(window_.get());
}

Window::~Window() {
  if (!window_->IsClosed())
    window_->CloseContents(nullptr);

  // Destroy the native window in next tick because the native code might be
  // iterating all windows.
  base::MessageLoop::current()->DeleteSoon(FROM_HERE, window_.release());
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::OnWindowClosed() {
  api_web_contents_->DestroyWebContents();

  RemoveFromWeakMap();
  window_->RemoveObserver(this);

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  Emit("closed");

  // Destroy the native class when window is closed.
  base::MessageLoop::current()->PostTask(FROM_HERE, GetDestroyClosure());
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
mate::WrappableBase* Window::New(v8::Isolate* isolate, mate::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
        isolate, "Cannot create BrowserWindow before app is ready")));
    return nullptr;
  }

  if (args->Length() > 1) {
    args->ThrowError();
    return nullptr;
  }

  mate::Dictionary options;
  if (!(args->Length() == 1 && args->GetNext(&options))) {
    options = mate::Dictionary::CreateEmpty(isolate);
  }

  return new Window(isolate, options);
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
  window_->ShowInactive();
}

void Window::Hide() {
  window_->Hide();
}

bool Window::IsVisible() {
  return window_->IsVisible();
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

void Window::SetAlwaysOnTop(bool top) {
  window_->SetAlwaysOnTop(top);
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

void Window::CapturePage(mate::Arguments* args) {
  gfx::Rect rect;
  base::Callback<void(const gfx::Image&)> callback;

  if (!(args->Length() == 1 && args->GetNext(&callback)) &&
      !(args->Length() == 2 && args->GetNext(&rect)
                            && args->GetNext(&callback))) {
    args->ThrowError();
    return;
  }

  window_->CapturePage(
      rect, base::Bind(&OnCapturePageDone, args->isolate(), callback));
}

void Window::SetProgressBar(double progress) {
  window_->SetProgressBar(progress);
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
      window->GetAcceleratedWidget(), buttons);
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
#endif

#if defined(OS_MACOSX)
void Window::ShowDefinitionForSelection() {
  window_->ShowDefinitionForSelection();
}
#endif

void Window::SetAspectRatio(double aspect_ratio, mate::Arguments* args) {
  gfx::Size extra_size;
  args->GetNext(&extra_size);
  window_->SetAspectRatio(aspect_ratio, extra_size);
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

int32_t Window::ID() const {
  return weak_map_id();
}

v8::Local<v8::Value> Window::WebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, web_contents_);
}

// static
void Window::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .MakeDestroyable()
      .SetMethod("close", &Window::Close)
      .SetMethod("focus", &Window::Focus)
      .SetMethod("blur", &Window::Blur)
      .SetMethod("isFocused", &Window::IsFocused)
      .SetMethod("show", &Window::Show)
      .SetMethod("showInactive", &Window::ShowInactive)
      .SetMethod("hide", &Window::Hide)
      .SetMethod("isVisible", &Window::IsVisible)
      .SetMethod("maximize", &Window::Maximize)
      .SetMethod("unmaximize", &Window::Unmaximize)
      .SetMethod("isMaximized", &Window::IsMaximized)
      .SetMethod("minimize", &Window::Minimize)
      .SetMethod("restore", &Window::Restore)
      .SetMethod("isMinimized", &Window::IsMinimized)
      .SetMethod("setFullScreen", &Window::SetFullScreen)
      .SetMethod("isFullScreen", &Window::IsFullscreen)
      .SetMethod("setAspectRatio", &Window::SetAspectRatio)
      .SetMethod("getNativeWindowHandle", &Window::GetNativeWindowHandle)
      .SetMethod("getBounds", &Window::GetBounds)
      .SetMethod("setBounds", &Window::SetBounds)
      .SetMethod("getSize", &Window::GetSize)
      .SetMethod("setSize", &Window::SetSize)
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
      .SetMethod("focusOnWebView", &Window::FocusOnWebView)
      .SetMethod("blurWebView", &Window::BlurWebView)
      .SetMethod("isWebViewFocused", &Window::IsWebViewFocused)
      .SetMethod("capturePage", &Window::CapturePage)
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
#if defined(OS_WIN)
      .SetMethod("hookWindowMessage", &Window::HookWindowMessage)
      .SetMethod("isWindowMessageHooked", &Window::IsWindowMessageHooked)
      .SetMethod("unhookWindowMessage", &Window::UnhookWindowMessage)
      .SetMethod("unhookAllWindowMessages", &Window::UnhookAllWindowMessages)
#endif
#if defined(OS_MACOSX)
      .SetMethod("showDefinitionForSelection",
                 &Window::ShowDefinitionForSelection)
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
  v8::Local<v8::Function> constructor = mate::CreateConstructor<Window>(
      isolate, "BrowserWindow", base::Bind(&Window::New));
  mate::Dictionary browser_window(isolate, constructor);
  browser_window.SetMethod("fromId",
                           &mate::TrackableObject<Window>::FromWeakMapID);
  browser_window.SetMethod("getAllWindows",
                           &mate::TrackableObject<Window>::GetAll);

  mate::Dictionary dict(isolate, exports);
  dict.Set("BrowserWindow", browser_window);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_window, Initialize)

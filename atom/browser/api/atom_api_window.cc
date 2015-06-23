// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_window.h"

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/callback.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/geometry/rect.h"

#include "atom/common/node_includes.h"

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

}  // namespace


Window::Window(const mate::Dictionary& options)
    : window_(NativeWindow::Create(options)) {
  window_->InitFromOptions(options);
  window_->AddObserver(this);
}

Window::~Window() {
  if (window_)
    Destroy();
}

void Window::OnPageTitleUpdated(bool* prevent_default,
                                const std::string& title) {
  *prevent_default = Emit("page-title-updated", title);
}

void Window::WillCreatePopupWindow(const base::string16& frame_name,
                                   const GURL& target_url,
                                   const std::string& partition_id,
                                   WindowOpenDisposition disposition) {
  Emit("-new-window", target_url, frame_name);
}

void Window::WillNavigate(bool* prevent_default, const GURL& url) {
  *prevent_default = Emit("-will-navigate", url);
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::OnWindowClosed() {
  Emit("closed");

  window_->RemoveObserver(this);
}

void Window::OnWindowBlur() {
  Emit("blur");
}

void Window::OnWindowFocus() {
  Emit("focus");
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

void Window::OnDevToolsFocus() {
  Emit("devtools-focused");
}

void Window::OnDevToolsOpened() {
  Emit("devtools-opened");

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto handle = WebContents::CreateFrom(isolate(),
                                        window_->GetDevToolsWebContents());
  devtools_web_contents_.Reset(isolate(), handle.ToV8());
}

void Window::OnDevToolsClosed() {
  Emit("devtools-closed");

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();
}

// static
mate::Wrappable* Window::New(v8::Isolate* isolate,
                             const mate::Dictionary& options) {
  if (!Browser::Get()->is_ready()) {
    node::ThrowError(isolate,
                     "Cannot create BrowserWindow before app is ready");
    return nullptr;
  }
  return new Window(options);
}

void Window::Destroy() {
  window_->DestroyWebContents();
  window_->CloseImmediately();
}

void Window::Close() {
  window_->Close();
}

bool Window::IsClosed() {
  return window_->IsClosed();
}

void Window::Focus() {
  window_->Focus(true);
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

void Window::SetBounds(const gfx::Rect& bounds) {
  window_->SetBounds(bounds);
}

gfx::Rect Window::GetBounds() {
  return window_->GetBounds();
}

void Window::SetSize(int width, int height) {
  window_->SetSize(gfx::Size(width, height));
}

std::vector<int> Window::GetSize() {
  std::vector<int> result(2);
  gfx::Size size = window_->GetSize();
  result[0] = size.width();
  result[1] = size.height();
  return result;
}

void Window::SetContentSize(int width, int height) {
  window_->SetContentSize(gfx::Size(width, height));
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

void Window::SetResizable(bool resizable) {
  window_->SetResizable(resizable);
}

bool Window::IsResizable() {
  return window_->IsResizable();
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

void Window::SetPosition(int x, int y) {
  window_->SetPosition(gfx::Point(x, y));
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

void Window::SetMenu(ui::SimpleMenuModel* menu) {
  window_->SetMenu(menu);
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

#if defined(OS_MACOSX)
void Window::ShowDefinitionForSelection() {
  window_->ShowDefinitionForSelection();
}
#endif

void Window::SetVisibleOnAllWorkspaces(bool visible) {
  return window_->SetVisibleOnAllWorkspaces(visible);
}

bool Window::IsVisibleOnAllWorkspaces() {
  return window_->IsVisibleOnAllWorkspaces();
}

v8::Local<v8::Value> Window::WebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty()) {
    auto handle =
        WebContents::CreateFrom(isolate, window_->managed_web_contents());
    web_contents_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, web_contents_);
}

v8::Local<v8::Value> Window::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

// static
void Window::BuildPrototype(v8::Isolate* isolate,
                            v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("destroy", &Window::Destroy)
      .SetMethod("close", &Window::Close)
      .SetMethod("isClosed", &Window::IsClosed)
      .SetMethod("focus", &Window::Focus)
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
      .SetMethod("setResizable", &Window::SetResizable)
      .SetMethod("isResizable", &Window::IsResizable)
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
      .SetMethod("setRepresentedFilename", &Window::SetRepresentedFilename)
      .SetMethod("getRepresentedFilename", &Window::GetRepresentedFilename)
      .SetMethod("setDocumentEdited", &Window::SetDocumentEdited)
      .SetMethod("isDocumentEdited", &Window::IsDocumentEdited)
      .SetMethod("focusOnWebView", &Window::FocusOnWebView)
      .SetMethod("blurWebView", &Window::BlurWebView)
      .SetMethod("isWebViewFocused", &Window::IsWebViewFocused)
      .SetMethod("capturePage", &Window::CapturePage)
      .SetMethod("setProgressBar", &Window::SetProgressBar)
      .SetMethod("setOverlayIcon", &Window::SetOverlayIcon)
      .SetMethod("_setMenu", &Window::SetMenu)
      .SetMethod("setAutoHideMenuBar", &Window::SetAutoHideMenuBar)
      .SetMethod("isMenuBarAutoHide", &Window::IsMenuBarAutoHide)
      .SetMethod("setMenuBarVisibility", &Window::SetMenuBarVisibility)
      .SetMethod("isMenuBarVisible", &Window::IsMenuBarVisible)
      .SetMethod("setVisibleOnAllWorkspaces",
                 &Window::SetVisibleOnAllWorkspaces)
      .SetMethod("isVisibleOnAllWorkspaces",
                 &Window::IsVisibleOnAllWorkspaces)
#if defined(OS_MACOSX)
      .SetMethod("showDefinitionForSelection",
                 &Window::ShowDefinitionForSelection)
#endif
      .SetProperty("webContents", &Window::WebContents)
      .SetProperty("devToolsWebContents", &Window::DevToolsWebContents);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  using atom::api::Window;
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<Window>(
      isolate, "BrowserWindow", base::Bind(&Window::New));
  mate::Dictionary dict(isolate, exports);
  dict.Set("BrowserWindow", static_cast<v8::Local<v8::Value>>(constructor));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_window, Initialize)

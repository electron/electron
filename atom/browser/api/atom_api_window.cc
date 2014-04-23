// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_window.h"

#include "atom/browser/native_window.h"
#include "atom/common/native_mate_converters/function_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/process/kill.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

#include "atom/common/node_includes.h"

using content::NavigationController;

namespace mate {

template<>
struct Converter<gfx::Rect> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     gfx::Rect* out) {
    if (!val->IsObject())
      return false;
    mate::Dictionary dict(isolate, val->ToObject());
    int x, y, width, height;
    if (!dict.Get("x", &x) || !dict.Get("y", &y) ||
        !dict.Get("width", &width) || !dict.Get("height", &height))
      return false;
    *out = gfx::Rect(x, y, width, height);
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {

void OnCapturePageDone(
    const base::Callback<void(v8::Handle<v8::Value>)>& callback,
    const std::vector<unsigned char>& data) {
  v8::Locker locker(node_isolate);
  v8::HandleScope handle_scope(node_isolate);

  v8::Local<v8::Value> buffer = node::Buffer::New(
      reinterpret_cast<const char*>(data.data()),
      data.size());
  callback.Run(buffer);
}

}  // namespace


Window::Window(base::DictionaryValue* options)
    : window_(NativeWindow::Create(options)) {
  window_->InitFromOptions(options);
  window_->AddObserver(this);
}

Window::~Window() {
  Emit("destroyed");
}

void Window::OnPageTitleUpdated(bool* prevent_default,
                                const std::string& title) {
  base::ListValue args;
  args.AppendString(title);
  *prevent_default = Emit("page-title-updated", args);
}

void Window::OnLoadingStateChanged(bool is_loading) {
  base::ListValue args;
  args.AppendBoolean(is_loading);
  Emit("loading-state-changed", args);
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::OnWindowClosed() {
  Emit("closed");

  if (window_) {
    window_->RemoveObserver(this);

    // Free memory when native window is closed, the delete is delayed so other
    // observers would not get a invalid pointer of NativeWindow.
    base::MessageLoop::current()->DeleteSoon(FROM_HERE, window_.release());
  }
}

void Window::OnWindowBlur() {
  Emit("blur");
}

void Window::OnRendererUnresponsive() {
  Emit("unresponsive");
}

void Window::OnRendererResponsive() {
  Emit("responsive");
}

void Window::OnRenderViewDeleted(int process_id, int routing_id) {
  base::ListValue args;
  args.AppendInteger(process_id);
  args.AppendInteger(routing_id);
  Emit("render-view-deleted", args);
}

void Window::OnRendererCrashed() {
  Emit("crashed");
}

// static
mate::Wrappable* Window::New(mate::Arguments* args,
                             const base::DictionaryValue& options) {
  scoped_ptr<base::DictionaryValue> copied_options(options.DeepCopy());
  Window* window = new Window(copied_options.get());
  window->Wrap(args->isolate(), args->GetThis());

  // Give js code a chance to do initialization.
  node::MakeCallback(args->GetThis(), "_init", 0, NULL);

  return window;
}

void Window::Destroy() {
  base::KillProcess(window_->GetRenderProcessHandle(), 0, false);
  window_->CloseImmediately();
}

void Window::Close() {
  window_->Close();
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

void Window::Minimize() {
  window_->Minimize();
}

void Window::Restore() {
  window_->Restore();
}

void Window::SetFullscreen(bool fullscreen) {
  window_->SetFullscreen(fullscreen);
}

bool Window::IsFullscreen() {
  return window_->IsFullscreen();
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

void Window::SetKiosk(bool kiosk) {
  window_->SetKiosk(kiosk);
}

bool Window::IsKiosk() {
  return window_->IsKiosk();
}

void Window::OpenDevTools() {
  window_->OpenDevTools();
}

void Window::CloseDevTools() {
  window_->CloseDevTools();
}

bool Window::IsDevToolsOpened() {
  return window_->IsDevToolsOpened();
}

void Window::InspectElement(int x, int y) {
  window_->InspectElement(x, y);
}

void Window::DebugDevTools() {
  if (window_->IsDevToolsOpened())
    NativeWindow::Debug(window_->GetDevToolsWebContents());
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

void Window::CapturePage(mate::Arguments* args) {
  gfx::Rect rect;
  base::Callback<void(v8::Handle<v8::Value>)> callback;

  if (!(args->Length() == 1 && args->GetNext(&callback)) ||
      !(args->Length() == 2 && args->GetNext(&rect)
                            && args->GetNext(&callback))) {
    args->ThrowError();
    return;
  }

  window_->CapturePage(rect, base::Bind(&OnCapturePageDone, callback));
}

string16 Window::GetPageTitle() {
  return window_->GetWebContents()->GetTitle();
}

bool Window::IsLoading() {
  return window_->GetWebContents()->IsLoading();
}

bool Window::IsWaitingForResponse() {
  return window_->GetWebContents()->IsWaitingForResponse();
}

void Window::Stop() {
  window_->GetWebContents()->Stop();
}

int Window::GetRoutingID() {
  return window_->GetWebContents()->GetRoutingID();
}

int Window::GetProcessID() {
  return window_->GetWebContents()->GetRenderProcessHost()->GetID();
}

bool Window::IsCrashed() {
  return window_->GetWebContents()->IsCrashed();
}

mate::Dictionary Window::GetDevTools(v8::Isolate* isolate) {
  content::WebContents* web_contents = window_->GetDevToolsWebContents();
  mate::Dictionary dict(isolate);
  dict.Set("processId", web_contents->GetRenderProcessHost()->GetID());
  dict.Set("routingId", web_contents->GetRoutingID());
  return dict;
}

void Window::ExecuteJavaScriptInDevTools(const std::string& code) {
  window_->ExecuteJavaScriptInDevTools(code);
}

void Window::LoadURL(const GURL& url) {
  NavigationController& controller = window_->GetWebContents()->GetController();

  content::NavigationController::LoadURLParams params(url);
  params.transition_type = content::PAGE_TRANSITION_TYPED;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  controller.LoadURLWithParams(params);
}

GURL Window::GetURL() {
  NavigationController& controller = window_->GetWebContents()->GetController();
  if (!controller.GetActiveEntry())
    return GURL();
  return controller.GetActiveEntry()->GetVirtualURL();
}

bool Window::CanGoBack() {
  return window_->GetWebContents()->GetController().CanGoBack();
}

bool Window::CanGoForward() {
  return window_->GetWebContents()->GetController().CanGoForward();
}

bool Window::CanGoToOffset(int offset) {
  return window_->GetWebContents()->GetController().CanGoToOffset(offset);
}

void Window::GoBack() {
  window_->GetWebContents()->GetController().GoBack();
}

void Window::GoForward() {
  window_->GetWebContents()->GetController().GoForward();
}

void Window::GoToIndex(int index) {
  window_->GetWebContents()->GetController().GoToIndex(index);
}

void Window::GoToOffset(int offset) {
  window_->GetWebContents()->GetController().GoToOffset(offset);
}

void Window::Reload() {
  window_->GetWebContents()->GetController().Reload(false);
}

void Window::ReloadIgnoringCache() {
  window_->GetWebContents()->GetController().ReloadIgnoringCache(false);
}

// static
void Window::BuildPrototype(v8::Isolate* isolate,
                            v8::Handle<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("destroy", &Window::Destroy)
      .SetMethod("close", &Window::Close)
      .SetMethod("focus", &Window::Focus)
      .SetMethod("isFocused", &Window::IsFocused)
      .SetMethod("show", &Window::Show)
      .SetMethod("hide", &Window::Hide)
      .SetMethod("isVisible", &Window::IsVisible)
      .SetMethod("maximize", &Window::Maximize)
      .SetMethod("unmaximize", &Window::Unmaximize)
      .SetMethod("minimize", &Window::Minimize)
      .SetMethod("restore", &Window::Restore)
      .SetMethod("setFullScreen", &Window::SetFullscreen)
      .SetMethod("isFullScreen", &Window::IsFullscreen)
      .SetMethod("getSize", &Window::GetSize)
      .SetMethod("setSize", &Window::SetSize)
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
      .SetMethod("setKiosk", &Window::SetKiosk)
      .SetMethod("isKiosk", &Window::IsKiosk)
      .SetMethod("openDevTools", &Window::OpenDevTools)
      .SetMethod("closeDevTools", &Window::CloseDevTools)
      .SetMethod("isDevToolsOpened", &Window::IsDevToolsOpened)
      .SetMethod("inspectElement", &Window::InspectElement)
      .SetMethod("debugDevTools", &Window::DebugDevTools)
      .SetMethod("focusOnWebView", &Window::FocusOnWebView)
      .SetMethod("blurWebView", &Window::BlurWebView)
      .SetMethod("isWebViewFocused", &Window::IsWebViewFocused)
      .SetMethod("capturePage", &Window::CapturePage)
      .SetMethod("getPageTitle", &Window::GetPageTitle)
      .SetMethod("isLoading", &Window::IsLoading)
      .SetMethod("isWaitingForResponse", &Window::IsWaitingForResponse)
      .SetMethod("stop", &Window::Stop)
      .SetMethod("getRoutingId", &Window::GetRoutingID)
      .SetMethod("getProcessId", &Window::GetProcessID)
      .SetMethod("isCrashed", &Window::IsCrashed)
      .SetMethod("getDevTools", &Window::GetDevTools)
      .SetMethod("executeJavaScriptInDevTools",
                 &Window::ExecuteJavaScriptInDevTools)
      .SetMethod("loadUrl", &Window::LoadURL)
      .SetMethod("getUrl", &Window::GetURL)
      .SetMethod("canGoBack", &Window::CanGoBack)
      .SetMethod("canGoForward", &Window::CanGoForward)
      .SetMethod("canGoToOffset", &Window::CanGoToOffset)
      .SetMethod("goBack", &Window::GoBack)
      .SetMethod("goForward", &Window::GoForward)
      .SetMethod("goToIndex", &Window::GoToIndex)
      .SetMethod("goToOffset", &Window::GoToOffset)
      .SetMethod("reload", &Window::Reload)
      .SetMethod("reloadIgnoringCache", &Window::ReloadIgnoringCache);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports) {
  using atom::api::Window;
  v8::Local<v8::Function> constructor = mate::CreateConstructor<Window>(
      node_isolate, "BrowserWindow", base::Bind(&Window::New));
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.Set("BrowserWindow", static_cast<v8::Handle<v8::Value>>(constructor));
}

}  // namespace

NODE_MODULE(atom_browser_window, Initialize)

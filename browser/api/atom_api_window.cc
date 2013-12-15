// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_window.h"

#include "base/process/kill.h"
#include "browser/native_window.h"
#include "common/v8/native_type_conversions.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_process_host.h"
#include "ui/gfx/point.h"
#include "ui/gfx/size.h"
#include "vendor/node/src/node_buffer.h"

#include "common/v8/node_common.h"

using content::NavigationController;
using node::ObjectWrap;

#define UNWRAP_WINDOW_AND_CHECK \
  Window* self = ObjectWrap::Unwrap<Window>(args.This()); \
  if (self == NULL) \
    return node::ThrowError("Window is already destroyed")

namespace atom {

namespace api {

Window::Window(v8::Handle<v8::Object> wrapper, base::DictionaryValue* options)
    : EventEmitter(wrapper),
      window_(NativeWindow::Create(options)) {
  window_->InitFromOptions(options);
  window_->AddObserver(this);
}

Window::~Window() {
  Emit("destroyed");

  window_->RemoveObserver(this);
}

void Window::OnPageTitleUpdated(bool* prevent_default,
                                const std::string& title) {
  base::ListValue args;
  args.AppendString(title);
  *prevent_default = Emit("page-title-updated", &args);
}

void Window::OnLoadingStateChanged(bool is_loading) {
  base::ListValue args;
  args.AppendBoolean(is_loading);
  Emit("loading-state-changed", &args);
}

void Window::WillCloseWindow(bool* prevent_default) {
  *prevent_default = Emit("close");
}

void Window::OnWindowClosed() {
  Emit("closed");

  // Free memory when native window is closed, the delete is delayed so other
  // observers would not get a invalid pointer of NativeWindow.
  base::MessageLoop::current()->DeleteSoon(FROM_HERE, this);
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
  Emit("render-view-deleted", &args);
}

void Window::OnRendererCrashed() {
  Emit("crashed");
}

void Window::OnCapturePageDone(const RefCountedV8Function& callback,
                               const std::vector<unsigned char>& data) {
  v8::HandleScope handle_scope(node_isolate);

  v8::Local<v8::Value> buffer = node::Buffer::New(
      reinterpret_cast<const char*>(data.data()),
      data.size());
  callback->NewHandle(node_isolate)->Call(
      v8::Context::GetCurrent()->Global(), 1, &buffer);
}

// static
void Window::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope handle_scope(args.GetIsolate());

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  scoped_ptr<base::Value> options;
  if (!FromV8Arguments(args, &options))
    return node::ThrowTypeError("Bad argument");

  if (!options || !options->IsType(base::Value::TYPE_DICTIONARY))
    return node::ThrowTypeError("Options must be dictionary");

  new Window(args.This(), static_cast<base::DictionaryValue*>(options.get()));

  // Give js code a chance to do initialization.
  node::MakeCallback(args.This(), "_init", 0, NULL);
}

// static
void Window::Destroy(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  base::ProcessHandle handle = self->window_->GetRenderProcessHandle();
  delete self;

  // Check if the render process is terminated, it could happen that the render
  // became a zombie.
  if (!base::WaitForSingleProcess(handle,
                                  base::TimeDelta::FromMilliseconds(500)))
    base::KillProcess(handle, 0, true);
}

// static
void Window::Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Close();
}

// static
void Window::Focus(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Focus(args[0]->IsBoolean() ? args[0]->BooleanValue(): true);
}

// static
void Window::IsFocused(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsFocused());
}

// static
void Window::Show(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Show();
}

// static
void Window::Hide(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Hide();
}

// static
void Window::IsVisible(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  return args.GetReturnValue().Set(self->window_->IsVisible());
}

// static
void Window::Maximize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Maximize();
}

// static
void Window::Unmaximize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Unmaximize();
}

// static
void Window::Minimize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Minimize();
}

// static
void Window::Restore(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Restore();
}

// static
void Window::SetFullscreen(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool fs;
  if (!FromV8Arguments(args, &fs))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetFullscreen(fs);
}

// static
void Window::IsFullscreen(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsFullscreen());
}

// static
void Window::SetSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int width, height;
  if (!FromV8Arguments(args, &width, &height))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetSize(gfx::Size(width, height));
}

// static
void Window::GetSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Size size = self->window_->GetSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(size.width()));
  ret->Set(1, ToV8Value(size.height()));

  args.GetReturnValue().Set(ret);
}

// static
void Window::SetMinimumSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int width, height;
  if (!FromV8Arguments(args, &width, &height))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetMinimumSize(gfx::Size(width, height));
}

// static
void Window::GetMinimumSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Size size = self->window_->GetMinimumSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(size.width()));
  ret->Set(1, ToV8Value(size.height()));

  args.GetReturnValue().Set(ret);
}

// static
void Window::SetMaximumSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int width, height;
  if (!FromV8Arguments(args, &width, &height))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetMaximumSize(gfx::Size(width, height));
}

// static
void Window::GetMaximumSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Size size = self->window_->GetMaximumSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(size.width()));
  ret->Set(1, ToV8Value(size.height()));

  args.GetReturnValue().Set(ret);
}

// static
void Window::SetResizable(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool resizable;
  if (!FromV8Arguments(args, &resizable))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetResizable(resizable);
}

// static
void Window::IsResizable(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsResizable());
}

// static
void Window::SetAlwaysOnTop(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool top;
  if (!FromV8Arguments(args, &top))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetAlwaysOnTop(top);
}

// static
void Window::IsAlwaysOnTop(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsAlwaysOnTop());
}

// static
void Window::Center(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->Center();
}

// static
void Window::SetPosition(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int x, y;
  if (!FromV8Arguments(args, &x, &y))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetPosition(gfx::Point(x, y));
}

// static
void Window::GetPosition(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Point pos = self->window_->GetPosition();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, ToV8Value(pos.x()));
  ret->Set(1, ToV8Value(pos.y()));

  args.GetReturnValue().Set(ret);
}

// static
void Window::SetTitle(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  std::string title;
  if (!FromV8Arguments(args, &title))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetTitle(title);
}

// static
void Window::GetTitle(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(ToV8Value(self->window_->GetTitle()));
}

// static
void Window::FlashFrame(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->FlashFrame(
      args[0]->IsBoolean() ? args[0]->BooleanValue(): true);
}

// static
void Window::SetKiosk(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  bool kiosk;
  if (!FromV8Arguments(args, &kiosk))
    return node::ThrowTypeError("Bad argument");

  self->window_->SetKiosk(kiosk);
}

// static
void Window::IsKiosk(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsKiosk());
}

// static
void Window::OpenDevTools(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->OpenDevTools();
}

// static
void Window::CloseDevTools(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->CloseDevTools();
}

// static
void Window::IsDevToolsOpened(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsDevToolsOpened());
}

// static
void Window::InspectElement(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int x, y;
  if (!FromV8Arguments(args, &x, &y))
    return node::ThrowTypeError("Bad argument");

  self->window_->InspectElement(x, y);
}

// static
void Window::FocusOnWebView(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->FocusOnWebView();
}

// static
void Window::BlurWebView(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->BlurWebView();
}

// static
void Window::IsWebViewFocused(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->IsWebViewFocused());
}

// static
void Window::CapturePage(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  gfx::Rect rect;
  RefCountedV8Function callback;
  if (!FromV8Arguments(args, &rect, &callback) &&
      !FromV8Arguments(args, &callback))
    return node::ThrowTypeError("Bad argument");

  self->window_->CapturePage(rect, base::Bind(&Window::OnCapturePageDone,
                                              base::Unretained(self),
                                              callback));
}

// static
void Window::GetPageTitle(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(ToV8Value(
      self->window_->GetWebContents()->GetTitle()));
}

// static
void Window::IsLoading(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->GetWebContents()->IsLoading());
}

// static
void Window::IsWaitingForResponse(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(
      self->window_->GetWebContents()->IsWaitingForResponse());
}

// static
void Window::Stop(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  self->window_->GetWebContents()->Stop();
}

// static
void Window::GetRoutingID(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->GetWebContents()->GetRoutingID());
}

// static
void Window::GetProcessID(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  args.GetReturnValue().Set(
      self->window_->GetWebContents()->GetRenderProcessHost()->GetID());
}

// static
void Window::IsCrashed(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;
  args.GetReturnValue().Set(self->window_->GetWebContents()->IsCrashed());
}

// static
void Window::LoadURL(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  GURL url;
  if (!FromV8Arguments(args, &url))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  content::NavigationController::LoadURLParams params(url);
  params.transition_type = content::PAGE_TRANSITION_TYPED;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  controller.LoadURLWithParams(params);
}

// static
void Window::GetURL(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  GURL url;
  if (controller.GetActiveEntry())
    url = controller.GetActiveEntry()->GetVirtualURL();

  args.GetReturnValue().Set(ToV8Value(url));
}

// static
void Window::CanGoBack(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  args.GetReturnValue().Set(controller.CanGoBack());
}

// static
void Window::CanGoForward(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  args.GetReturnValue().Set(controller.CanGoForward());
}

// static
void Window::CanGoToOffset(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int offset;
  if (!FromV8Arguments(args, &offset))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  args.GetReturnValue().Set(controller.CanGoToOffset(offset));
}

// static
void Window::GoBack(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoBack();
}

// static
void Window::GoForward(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoForward();
}

// static
void Window::GoToIndex(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int index;
  if (!FromV8Arguments(args, &index))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoToIndex(index);
}

// static
void Window::GoToOffset(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  int offset;
  if (!FromV8Arguments(args, &offset))
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoToOffset(offset);
}

// static
void Window::Reload(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.Reload(args[0]->IsBoolean() ? args[0]->BooleanValue(): false);
}

// static
void Window::ReloadIgnoringCache(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_WINDOW_AND_CHECK;

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.ReloadIgnoringCache(
      args[0]->IsBoolean() ? args[0]->BooleanValue(): false);
}

// static
void Window::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(node_isolate);

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(Window::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("BrowserWindow"));

  NODE_SET_PROTOTYPE_METHOD(t, "destroy", Destroy);
  NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
  NODE_SET_PROTOTYPE_METHOD(t, "focus", Focus);
  NODE_SET_PROTOTYPE_METHOD(t, "isFocused", IsFocused);
  NODE_SET_PROTOTYPE_METHOD(t, "show", Show);
  NODE_SET_PROTOTYPE_METHOD(t, "hide", Hide);
  NODE_SET_PROTOTYPE_METHOD(t, "isVisible", IsVisible);
  NODE_SET_PROTOTYPE_METHOD(t, "maximize", Maximize);
  NODE_SET_PROTOTYPE_METHOD(t, "unmaximize", Unmaximize);
  NODE_SET_PROTOTYPE_METHOD(t, "minimize", Minimize);
  NODE_SET_PROTOTYPE_METHOD(t, "restore", Restore);
  NODE_SET_PROTOTYPE_METHOD(t, "setFullScreen", SetFullscreen);
  NODE_SET_PROTOTYPE_METHOD(t, "isFullScreen", IsFullscreen);
  NODE_SET_PROTOTYPE_METHOD(t, "setSize", SetSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getSize", GetSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setMinimumSize", SetMinimumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getMinimumSize", GetMinimumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaximumSize", SetMaximumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getMaximumSize", GetMaximumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setResizable", SetResizable);
  NODE_SET_PROTOTYPE_METHOD(t, "isResizable", IsResizable);
  NODE_SET_PROTOTYPE_METHOD(t, "setAlwaysOnTop", SetAlwaysOnTop);
  NODE_SET_PROTOTYPE_METHOD(t, "isAlwaysOnTop", IsAlwaysOnTop);
  NODE_SET_PROTOTYPE_METHOD(t, "center", Center);
  NODE_SET_PROTOTYPE_METHOD(t, "setPosition", SetPosition);
  NODE_SET_PROTOTYPE_METHOD(t, "getPosition", GetPosition);
  NODE_SET_PROTOTYPE_METHOD(t, "setTitle", SetTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "getTitle", GetTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "flashFrame", FlashFrame);
  NODE_SET_PROTOTYPE_METHOD(t, "setKiosk", SetKiosk);
  NODE_SET_PROTOTYPE_METHOD(t, "isKiosk", IsKiosk);
  NODE_SET_PROTOTYPE_METHOD(t, "openDevTools", OpenDevTools);
  NODE_SET_PROTOTYPE_METHOD(t, "closeDevTools", CloseDevTools);
  NODE_SET_PROTOTYPE_METHOD(t, "isDevToolsOpened", IsDevToolsOpened);
  NODE_SET_PROTOTYPE_METHOD(t, "inspectElement", InspectElement);
  NODE_SET_PROTOTYPE_METHOD(t, "focusOnWebView", FocusOnWebView);
  NODE_SET_PROTOTYPE_METHOD(t, "blurWebView", BlurWebView);
  NODE_SET_PROTOTYPE_METHOD(t, "isWebViewFocused", IsWebViewFocused);
  NODE_SET_PROTOTYPE_METHOD(t, "capturePage", CapturePage);

  NODE_SET_PROTOTYPE_METHOD(t, "getPageTitle", GetPageTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "isLoading", IsLoading);
  NODE_SET_PROTOTYPE_METHOD(t, "isWaitingForResponse", IsWaitingForResponse);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", Stop);
  NODE_SET_PROTOTYPE_METHOD(t, "getRoutingId", GetRoutingID);
  NODE_SET_PROTOTYPE_METHOD(t, "getProcessId", GetProcessID);
  NODE_SET_PROTOTYPE_METHOD(t, "isCrashed", IsCrashed);

  NODE_SET_PROTOTYPE_METHOD(t, "loadUrl", LoadURL);
  NODE_SET_PROTOTYPE_METHOD(t, "getUrl", GetURL);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoBack", CanGoBack);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoForward", CanGoForward);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoToOffset", CanGoToOffset);
  NODE_SET_PROTOTYPE_METHOD(t, "goBack", GoBack);
  NODE_SET_PROTOTYPE_METHOD(t, "goForward", GoForward);
  NODE_SET_PROTOTYPE_METHOD(t, "goToIndex", GoToIndex);
  NODE_SET_PROTOTYPE_METHOD(t, "goToOffset", GoToOffset);
  NODE_SET_PROTOTYPE_METHOD(t, "reload", Reload);
  NODE_SET_PROTOTYPE_METHOD(t, "reloadIgnoringCache", ReloadIgnoringCache);

  target->Set(v8::String::NewSymbol("BrowserWindow"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_window, atom::api::Window::Initialize)

// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_window.h"

#include "base/values.h"
#include "browser/atom_browser_context.h"
#include "browser/native_window.h"
#include "common/v8_value_converter_impl.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

using content::V8ValueConverter;
using content::NavigationController;
using node::ObjectWrap;

namespace atom {

namespace api {

namespace {

// Converts string16 to V8 String.
v8::Handle<v8::String> UTF16ToV8String(const string16& s) {
  return v8::String::New(reinterpret_cast<const uint16_t*>(s.data()), s.size());
}

}

Window::Window(v8::Handle<v8::Object> wrapper, base::DictionaryValue* options)
    : EventEmitter(wrapper),
      window_(NativeWindow::Create(AtomBrowserContext::Get(), options)) {
  window_->InitFromOptions(options);
}

Window::~Window() {
}

// static
v8::Handle<v8::Value> Window::New(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsObject())
    return node::ThrowTypeError("Bad argument");

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverterImpl());
  scoped_ptr<base::Value> options(
      converter->FromV8Value(args[0], v8::Context::GetCurrent()));

  if (!options || !options->IsType(base::Value::TYPE_DICTIONARY))
    return node::ThrowTypeError("Bad argument");

  new Window(args.This(), static_cast<base::DictionaryValue*>(options.get()));

  return args.This();
}

// static
v8::Handle<v8::Value> Window::Destroy(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  delete self;

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Close(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Close();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Focus(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Focus(args[0]->IsBoolean() ? args[0]->BooleanValue(): true);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Show(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Show();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Hide(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Hide();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Maximize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Maximize();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Unmaximize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Unmaximize();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Minimize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Minimize();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Restore(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->Restore();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetFullscreen(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1 || !args[0]->IsBoolean())
    return node::ThrowTypeError("Bad argument");
  self->window_->SetFullscreen(args[0]->BooleanValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsFullscreen(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  return v8::Boolean::New(self->window_->IsFullscreen());
}

// static
v8::Handle<v8::Value> Window::SetSize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 2)
    return node::ThrowTypeError("Bad argument");
  self->window_->SetSize(
      gfx::Size(args[0]->IntegerValue(), args[1]->IntegerValue()));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetSize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  gfx::Size size = self->window_->GetSize();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, v8::Integer::New(size.width()));
  ret->Set(1, v8::Integer::New(size.height()));

  return ret;
}

// static
v8::Handle<v8::Value> Window::SetMinimumSize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 2)
    return node::ThrowTypeError("Bad argument");
  self->window_->SetMinimumSize(
      gfx::Size(args[0]->IntegerValue(), args[1]->IntegerValue()));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetMaximumSize(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 2)
    return node::ThrowTypeError("Bad argument");
  self->window_->SetMaximumSize(
      gfx::Size(args[0]->IntegerValue(), args[1]->IntegerValue()));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetResizable(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1 || !args[0]->IsBoolean())
    return node::ThrowTypeError("Bad argument");
  self->window_->SetResizable(args[0]->BooleanValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetAlwaysOnTop(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1 || !args[0]->IsBoolean())
    return node::ThrowTypeError("Bad argument");
  self->window_->SetAlwaysOnTop(args[0]->BooleanValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetPosition(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 2)
    return node::ThrowTypeError("Bad argument");
  self->window_->SetPosition(
      gfx::Point(args[0]->IntegerValue(), args[1]->IntegerValue()));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetPosition(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  gfx::Point pos = self->window_->GetPosition();
  v8::Handle<v8::Array> ret = v8::Array::New(2);
  ret->Set(0, v8::Integer::New(pos.x()));
  ret->Set(1, v8::Integer::New(pos.y()));

  return ret;
}

// static
v8::Handle<v8::Value> Window::SetTitle(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1 || !args[0]->IsString())
    return node::ThrowTypeError("Bad argument");
  self->window_->SetTitle(*v8::String::Utf8Value(args[0]));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetTitle(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  std::string title = self->window_->GetTitle();

  return v8::String::New(title.c_str(), title.size());
}

// static
v8::Handle<v8::Value> Window::FlashFrame(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->FlashFrame(
      args[0]->IsBoolean() ? args[0]->BooleanValue(): true);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::SetKiosk(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1 || !args[0]->IsBoolean())
    return node::ThrowTypeError("Bad argument");
  self->window_->SetKiosk(args[0]->BooleanValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::IsKiosk(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  return v8::Boolean::New(self->window_->IsKiosk());
}

// static
v8::Handle<v8::Value> Window::ShowDevTools(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->ShowDevTools();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::CloseDevTools(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->CloseDevTools();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GetPageTitle(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  string16 title = self->window_->GetWebContents()->GetTitle();

  return UTF16ToV8String(title);
}

// static
v8::Handle<v8::Value> Window::IsLoading(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  return v8::Boolean::New(self->window_->GetWebContents()->IsLoading());
}

// static
v8::Handle<v8::Value> Window::IsWaitingForResponse(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  return v8::Boolean::New(
      self->window_->GetWebContents()->IsWaitingForResponse());
}

// static
v8::Handle<v8::Value> Window::Stop(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  self->window_->GetWebContents()->Stop();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::LoadURL(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1 || !args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.LoadURL(GURL(*v8::String::Utf8Value(args[0])),
                     content::Referrer(),
                     content::PAGE_TRANSITION_AUTO_TOPLEVEL,
                     std::string());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::CanGoBack(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  return v8::Boolean::New(controller.CanGoBack());
}

// static
v8::Handle<v8::Value> Window::CanGoForward(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();

  return v8::Boolean::New(controller.CanGoForward());
}

// static
v8::Handle<v8::Value> Window::CanGoToOffset(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1)
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  int offset = args[0]->IntegerValue();

  return v8::Boolean::New(controller.CanGoToOffset(offset));
}

// static
v8::Handle<v8::Value> Window::GoBack(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoBack();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GoForward(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoForward();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GoToIndex(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1)
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoToIndex(args[0]->IntegerValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::GoToOffset(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  if (args.Length() < 1)
    return node::ThrowTypeError("Bad argument");

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.GoToOffset(args[0]->IntegerValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::Reload(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.Reload(args[0]->IsBoolean() ? args[0]->BooleanValue(): false);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Window::ReloadIgnoringCache(const v8::Arguments &args) {
  Window* self = ObjectWrap::Unwrap<Window>(args.This());

  NavigationController& controller =
      self->window_->GetWebContents()->GetController();
  controller.ReloadIgnoringCache(
      args[0]->IsBoolean() ? args[0]->BooleanValue(): false);

  return v8::Undefined();
}

// static
void Window::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(Window::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Window"));

  NODE_SET_PROTOTYPE_METHOD(t, "destroy", Destroy);
  NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
  NODE_SET_PROTOTYPE_METHOD(t, "focus", Focus);
  NODE_SET_PROTOTYPE_METHOD(t, "show", Show);
  NODE_SET_PROTOTYPE_METHOD(t, "hide", Hide);
  NODE_SET_PROTOTYPE_METHOD(t, "maximize", Maximize);
  NODE_SET_PROTOTYPE_METHOD(t, "unmaximize", Unmaximize);
  NODE_SET_PROTOTYPE_METHOD(t, "minimize", Minimize);
  NODE_SET_PROTOTYPE_METHOD(t, "restore", Restore);
  NODE_SET_PROTOTYPE_METHOD(t, "setFullscreen", SetFullscreen);
  NODE_SET_PROTOTYPE_METHOD(t, "isFullscreen", IsFullscreen);
  NODE_SET_PROTOTYPE_METHOD(t, "setSize", SetSize);
  NODE_SET_PROTOTYPE_METHOD(t, "getSize", GetSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setMinimumSize", SetMinimumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setMaximumSize", SetMaximumSize);
  NODE_SET_PROTOTYPE_METHOD(t, "setResizable", SetResizable);
  NODE_SET_PROTOTYPE_METHOD(t, "setAlwaysOnTop", SetAlwaysOnTop);
  NODE_SET_PROTOTYPE_METHOD(t, "setPosition", SetPosition);
  NODE_SET_PROTOTYPE_METHOD(t, "getPosition", GetPosition);
  NODE_SET_PROTOTYPE_METHOD(t, "setTitle", SetTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "getTitle", GetTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "flashFrame", FlashFrame);
  NODE_SET_PROTOTYPE_METHOD(t, "setKiosk", SetKiosk);
  NODE_SET_PROTOTYPE_METHOD(t, "isKiosk", IsKiosk);
  NODE_SET_PROTOTYPE_METHOD(t, "showDevTools", ShowDevTools);
  NODE_SET_PROTOTYPE_METHOD(t, "closeDevTools", CloseDevTools);

  NODE_SET_PROTOTYPE_METHOD(t, "getPageTitle", GetPageTitle);
  NODE_SET_PROTOTYPE_METHOD(t, "isLoading", IsLoading);
  NODE_SET_PROTOTYPE_METHOD(t, "isWaitingForResponse", IsWaitingForResponse);
  NODE_SET_PROTOTYPE_METHOD(t, "stop", Stop);

  NODE_SET_PROTOTYPE_METHOD(t, "loadURL", LoadURL);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoBack", CanGoBack);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoForward", CanGoForward);
  NODE_SET_PROTOTYPE_METHOD(t, "canGoToOffset", CanGoToOffset);
  NODE_SET_PROTOTYPE_METHOD(t, "goBack", GoBack);
  NODE_SET_PROTOTYPE_METHOD(t, "goForward", GoForward);
  NODE_SET_PROTOTYPE_METHOD(t, "goToIndex", GoToIndex);
  NODE_SET_PROTOTYPE_METHOD(t, "goToOffset", GoToOffset);
  NODE_SET_PROTOTYPE_METHOD(t, "reload", Reload);
  NODE_SET_PROTOTYPE_METHOD(t, "reloadIgnoringCache", ReloadIgnoringCache);

  target->Set(v8::String::NewSymbol("Window"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_api_window, atom::api::Window::Initialize)

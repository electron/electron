// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_app.h"

#include <string>

#include "base/values.h"
#include "base/command_line.h"
#include "atom/browser/browser.h"
#include "atom/common/v8/native_type_conversions.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

App::App(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper) {
  Browser::Get()->AddObserver(this);
}

App::~App() {
  Browser::Get()->RemoveObserver(this);
}

void App::OnWillQuit(bool* prevent_default) {
  *prevent_default = Emit("will-quit");
}

void App::OnWindowAllClosed() {
  Emit("window-all-closed");
}

void App::OnOpenFile(bool* prevent_default, const std::string& file_path) {
  base::ListValue args;
  args.AppendString(file_path);
  *prevent_default = Emit("open-file", &args);
}

void App::OnOpenURL(const std::string& url) {
  base::ListValue args;
  args.AppendString(url);
  Emit("open-url", &args);
}

void App::OnActivateWithNoOpenWindows() {
  Emit("activate-with-no-open-windows");
}

void App::OnWillFinishLaunching() {
  Emit("will-finish-launching");
}

void App::OnFinishLaunching() {
  Emit("ready");
}

// static
void App::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope scope(args.GetIsolate());

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  new App(args.This());
}

// static
void App::Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Browser::Get()->Quit();
}

// static
void App::Focus(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Browser::Get()->Focus();
}

// static
void App::GetVersion(const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(ToV8Value(Browser::Get()->GetVersion()));
}

// static
void App::SetVersion(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string version;
  if (!FromV8Arguments(args, &version))
    return node::ThrowError("Bad argument");

  Browser::Get()->SetVersion(version);
}

// static
void App::GetName(const v8::FunctionCallbackInfo<v8::Value>& args) {
  return args.GetReturnValue().Set(ToV8Value(Browser::Get()->GetName()));
}

// static
void App::SetName(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string name;
  if (!FromV8Arguments(args, &name))
    return node::ThrowError("Bad argument");

  Browser::Get()->SetName(name);
}

// static
void App::AppendSwitch(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string switch_string;
  if (!FromV8Arguments(args, &switch_string))
    return node::ThrowError("Bad argument");

  if (args.Length() == 1) {
    CommandLine::ForCurrentProcess()->AppendSwitch(switch_string);
  } else {
    std::string value = FromV8Value(args[1]);
    CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switch_string, value);
  }
}

// static
void App::AppendArgument(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string value;
  if (!FromV8Arguments(args, &value))
    return node::ThrowError("Bad argument");

  CommandLine::ForCurrentProcess()->AppendArg(value);
}

#if defined(OS_MACOSX)

// static
void App::DockBounce(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string type;
  if (!FromV8Arguments(args, &type))
    return node::ThrowError("Bad argument");

  int request_id = -1;

  if (type == "critical")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_CRITICAL);
  else if (type == "informational")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_INFORMATIONAL);
  else
    return node::ThrowTypeError("Invalid bounce type");

  args.GetReturnValue().Set(request_id);
}

// static
void App::DockCancelBounce(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Browser::Get()->DockCancelBounce(FromV8Value(args[0]));
}

// static
void App::DockSetBadgeText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Browser::Get()->DockSetBadgeText(FromV8Value(args[0]));
}

// static
void App::DockGetBadgeText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string text(Browser::Get()->DockGetBadgeText());
  args.GetReturnValue().Set(ToV8Value(text));
}

#endif  // defined(OS_MACOSX)

// static
void App::Initialize(v8::Handle<v8::Object> target) {
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Application"));

  NODE_SET_PROTOTYPE_METHOD(t, "quit", Quit);
  NODE_SET_PROTOTYPE_METHOD(t, "focus", Focus);
  NODE_SET_PROTOTYPE_METHOD(t, "getVersion", GetVersion);
  NODE_SET_PROTOTYPE_METHOD(t, "setVersion", SetVersion);
  NODE_SET_PROTOTYPE_METHOD(t, "getName", GetName);
  NODE_SET_PROTOTYPE_METHOD(t, "setName", SetName);

  target->Set(v8::String::NewSymbol("Application"), t->GetFunction());

  NODE_SET_METHOD(target, "appendSwitch", AppendSwitch);
  NODE_SET_METHOD(target, "appendArgument", AppendArgument);

#if defined(OS_MACOSX)
  NODE_SET_METHOD(target, "dockBounce", DockBounce);
  NODE_SET_METHOD(target, "dockCancelBounce", DockCancelBounce);
  NODE_SET_METHOD(target, "dockSetBadgeText", DockSetBadgeText);
  NODE_SET_METHOD(target, "dockGetBadgeText", DockGetBadgeText);
#endif  // defined(OS_MACOSX)
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_app, atom::api::App::Initialize)

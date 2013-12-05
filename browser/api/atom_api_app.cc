// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_app.h"

#include "base/values.h"
#include "base/command_line.h"
#include "browser/browser.h"
#include "common/v8_conversions.h"
#include "vendor/node/src/node.h"

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

void App::OnWillFinishLaunching() {
  Emit("will-finish-launching");
}

void App::OnFinishLaunching() {
  Emit("finish-launching");
}

// static
v8::Handle<v8::Value> App::New(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  new App(args.This());

  return args.This();
}

// static
v8::Handle<v8::Value> App::Quit(const v8::Arguments &args) {
  v8::HandleScope scope;

  Browser::Get()->Quit();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::Exit(const v8::Arguments &args) {
  v8::HandleScope scope;

  exit(args[0]->IntegerValue());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::Terminate(const v8::Arguments &args) {
  v8::HandleScope scope;

  Browser::Get()->Terminate();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::Focus(const v8::Arguments &args) {
  v8::HandleScope scope;

  Browser::Get()->Focus();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::GetVersion(const v8::Arguments &args) {
  v8::HandleScope scope;

  std::string version(Browser::Get()->GetVersion());

  return v8::String::New(version.data(), version.size());
}

// static
v8::Handle<v8::Value> App::SetVersion(const v8::Arguments &args) {
  v8::HandleScope scope;

  std::string version;
  if (!FromV8Arguments(args, &version))
    return node::ThrowError("Bad argument");

  Browser::Get()->SetVersion(version);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::GetName(const v8::Arguments &args) {
  v8::HandleScope scope;

  std::string name(Browser::Get()->GetName());

  return v8::String::New(name.data(), version.size());
}

// static
v8::Handle<v8::Value> App::SetName(const v8::Arguments &args) {
  v8::HandleScope scope;

  std::string name;
  if (!FromV8Arguments(args, &name))
    return node::ThrowError("Bad argument");

  Browser::Get()->SetName(name);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::AppendSwitch(const v8::Arguments &args) {
  v8::HandleScope scope;

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

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::AppendArgument(const v8::Arguments &args) {
  v8::HandleScope scope;

  std::string value;
  if (!FromV8Arguments(args, &value))
    return node::ThrowError("Bad argument");

  CommandLine::ForCurrentProcess()->AppendArg(value);

  return v8::Undefined();
}

#if defined(OS_MACOSX)

// static
v8::Handle<v8::Value> App::DockBounce(const v8::Arguments& args) {
  std::string type = FromV8Value(args[0]);
  int request_id = -1;

  if (type == "critical")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_CRITICAL);
  else if (type == "informational")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_INFORMATIONAL);
  else
    return node::ThrowTypeError("Invalid bounce type");

  return v8::Integer::New(request_id);
}

// static
v8::Handle<v8::Value> App::DockCancelBounce(const v8::Arguments& args) {
  Browser::Get()->DockCancelBounce(FromV8Value(args[0]));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::DockSetBadgeText(const v8::Arguments& args) {
  Browser::Get()->DockSetBadgeText(FromV8Value(args[0]));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::DockGetBadgeText(const v8::Arguments& args) {
  std::string text(Browser::Get()->DockGetBadgeText());
  return ToV8Value(text);
}

#endif  // defined(OS_MACOSX)

// static
void App::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(App::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Application"));

  NODE_SET_PROTOTYPE_METHOD(t, "quit", Quit);
  NODE_SET_PROTOTYPE_METHOD(t, "exit", Exit);
  NODE_SET_PROTOTYPE_METHOD(t, "terminate", Terminate);
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

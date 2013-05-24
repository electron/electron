// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_app.h"

#include "base/command_line.h"
#include "browser/browser.h"
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
v8::Handle<v8::Value> App::AppendSwitch(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowError("Bad argument");

  std::string switch_string(*v8::String::Utf8Value(args[0]));
  if (args.Length() == 1) {
    CommandLine::ForCurrentProcess()->AppendSwitch(switch_string);
  } else {
    std::string value(*v8::String::Utf8Value(args[1]));
    CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switch_string, value);
  }

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> App::AppendArgument(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowError("Bad argument");

  std::string value(*v8::String::Utf8Value(args[0]));
  CommandLine::ForCurrentProcess()->AppendArg(value);

  return v8::Undefined();
}

// static
void App::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(App::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Application"));

  NODE_SET_PROTOTYPE_METHOD(t, "quit", Quit);
  NODE_SET_PROTOTYPE_METHOD(t, "exit", Exit);
  NODE_SET_PROTOTYPE_METHOD(t, "terminate", Terminate);

  target->Set(v8::String::NewSymbol("Application"), t->GetFunction());

  NODE_SET_METHOD(target, "appendSwitch", AppendSwitch);
  NODE_SET_METHOD(target, "appendArgument", AppendArgument);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_app, atom::api::App::Initialize)

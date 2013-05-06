// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_menu.h"

namespace atom {

namespace api {

Menu::Menu(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper),
      model_(new ui::SimpleMenuModel(this)) {
}

Menu::~Menu() {
}

bool Menu::IsCommandIdChecked(int command_id) const {
  return false;
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  return true;
}

bool Menu::GetAcceleratorForCommandId(int command_id,
                                    ui::Accelerator* accelerator) {
  return false;
}

void Menu::ExecuteCommand(int command_id, int event_flags) {
}

// static
v8::Handle<v8::Value> Menu::New(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  Menu::Create(args.This());

  return args.This();
}

// static
void Menu::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t(v8::FunctionTemplate::New(Menu::New));
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Menu"));

  target->Set(v8::String::NewSymbol("Menu"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_menu, atom::api::Menu::Initialize)

// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_menu.h"

#define UNWRAP_MEMNU_AND_CHECK \
  Menu* self = ObjectWrap::Unwrap<Menu>(args.This()); \
  if (self == NULL) \
    return node::ThrowError("Menu is already destroyed")

namespace atom {

namespace api {

namespace {

// Converts a V8 value to a string16.
string16 V8ValueToUTF16(v8::Handle<v8::Value> value) {
  v8::String::Value s(value);
  return string16(reinterpret_cast<const char16*>(*s), s.length());
}

// Converts string16 to V8 String.
v8::Handle<v8::Value> UTF16ToV8Value(const string16& s) {
  return v8::String::New(reinterpret_cast<const uint16_t*>(s.data()), s.size());
}

}  // namespace

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

bool Menu::IsCommandIdVisible(int command_id) const {
  return true;
}

bool Menu::GetAcceleratorForCommandId(int command_id,
                                    ui::Accelerator* accelerator) {
  return false;
}

bool Menu::IsItemForCommandIdDynamic(int command_id) const {
  return false;
}

string16 Menu::GetLabelForCommandId(int command_id) const {
  return string16();
}

string16 Menu::GetSublabelForCommandId(int command_id) const {
  return string16();
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
v8::Handle<v8::Value> Menu::InsertItem(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString())
    return node::ThrowTypeError("Bad argument");

  int index = args[0]->IntegerValue();

  if (index < 0)
    self->model_->AddItem(args[1]->IntegerValue(), V8ValueToUTF16(args[2]));
  else
    self->model_->InsertItemAt(
        index, args[1]->IntegerValue(), V8ValueToUTF16(args[2]));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertCheckItem(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString())
    return node::ThrowTypeError("Bad argument");

  int index = args[0]->IntegerValue();
  int command_id = args[1]->IntegerValue();

  if (index < 0)
    self->model_->AddCheckItem(command_id, V8ValueToUTF16(args[2]));
  else
    self->model_->InsertCheckItemAt(index, command_id, V8ValueToUTF16(args[2]));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertRadioItem(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsNumber() ||
      !args[1]->IsNumber() ||
      !args[2]->IsString() ||
      !args[3]->IsNumber())
    return node::ThrowTypeError("Bad argument");

  int index = args[0]->IntegerValue();
  int command_id = args[1]->IntegerValue();
  int group_id = args[3]->IntegerValue();

  if (index < 0)
    self->model_->AddRadioItem(command_id, V8ValueToUTF16(args[2]), group_id);
  else
    self->model_->InsertRadioItemAt(
        index, command_id, V8ValueToUTF16(args[2]), group_id);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertSeparator(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsNumber() || !args[1]->IsNumber() || !args[2]->IsString())
    return node::ThrowTypeError("Bad argument");

  int index = args[0]->IntegerValue();

  if (index < 0)
    self->model_->AddSeparator(ui::NORMAL_SEPARATOR);
  else
    self->model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertSubMenu(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  // FIXME should rely on js code to keep a reference of submenu and check
  // the constructor type of menu object.

  if (!args[0]->IsNumber() ||
      !args[1]->IsNumber() ||
      !args[2]->IsString() ||
      !args[3]->IsObject())
    return node::ThrowTypeError("Bad argument");

  Menu* submenu = ObjectWrap::Unwrap<Menu>(args[3]->ToObject());
  if (!submenu)
    return node::ThrowTypeError("The submenu is already destroyed");

  int index = args[0]->IntegerValue();
  int command_id = args[1]->IntegerValue();

  if (index < 0)
    self->model_->AddSubMenu(
        command_id, V8ValueToUTF16(args[2]), submenu->model_.get());
  else
    self->model_->InsertSubMenuAt(
        index, command_id, V8ValueToUTF16(args[2]), submenu->model_.get());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::SetIcon(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsNumber() || !args[1]->IsString())
    return node::ThrowTypeError("Bad argument");

  // FIXME use webkit_glue's image decoder here.

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::SetSublabel(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsNumber() || !args[1]->IsString())
    return node::ThrowTypeError("Bad argument");

  self->model_->SetSublabel(args[0]->IntegerValue(), V8ValueToUTF16(args[1]));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::Clear(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  self->model_->Clear();

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::GetIndexOfCommandId(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = args[0]->IntegerValue();
  return v8::Integer::New(self->model_->GetIndexOfCommandId(index));
}

// static
v8::Handle<v8::Value> Menu::GetItemCount(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  return v8::Integer::New(self->model_->GetItemCount());
}

// static
v8::Handle<v8::Value> Menu::GetCommandIdAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = args[0]->IntegerValue();
  return v8::Integer::New(self->model_->GetCommandIdAt(index));
}

// static
v8::Handle<v8::Value> Menu::GetLabelAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = args[0]->IntegerValue();
  return UTF16ToV8Value(self->model_->GetLabelAt(index));
}

// static
v8::Handle<v8::Value> Menu::GetSublabelAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = args[0]->IntegerValue();
  return UTF16ToV8Value(self->model_->GetSublabelAt(index));
}

// static
v8::Handle<v8::Value> Menu::IsItemCheckedAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = args[0]->IntegerValue();
  return v8::Boolean::New(self->model_->IsItemCheckedAt(index));
}

// static
v8::Handle<v8::Value> Menu::IsEnabledAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  return v8::Boolean::New(self->model_->IsEnabledAt(args[0]->IntegerValue()));
}

// static
v8::Handle<v8::Value> Menu::IsVisibleAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  return v8::Boolean::New(self->model_->IsVisibleAt(args[0]->IntegerValue()));
}

// static
v8::Handle<v8::Value> Menu::Popup(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  if (!args[0]->IsObject() || !args[1]->IsNumber() || !args[2]->IsNumber())
    return node::ThrowTypeError("Bad argument");

  NativeWindow* window = Unwrap<NativeWindow>(args[0]->ToObject());
  if (!window)
    return node::ThrowTypeError("Invalid window");

  self->Popup(window, args[1]->IntegerValue(), args[2]->IntegerValue());

  return v8::Undefined();
}

// static
void Menu::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t(v8::FunctionTemplate::New(Menu::New));
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Menu"));

  NODE_SET_PROTOTYPE_METHOD(t, "insertItem", InsertItem);
  NODE_SET_PROTOTYPE_METHOD(t, "insertCheckItem", InsertCheckItem);
  NODE_SET_PROTOTYPE_METHOD(t, "insertRadioItem", InsertRadioItem);
  NODE_SET_PROTOTYPE_METHOD(t, "insertSeparator", InsertSeparator);
  NODE_SET_PROTOTYPE_METHOD(t, "insertSubMenu", InsertSubMenu);

  NODE_SET_PROTOTYPE_METHOD(t, "setIcon", SetIcon);
  NODE_SET_PROTOTYPE_METHOD(t, "setSublabel", SetSublabel);

  NODE_SET_PROTOTYPE_METHOD(t, "clear", Clear);

  NODE_SET_PROTOTYPE_METHOD(t, "getIndexOfCommandId", GetIndexOfCommandId);
  NODE_SET_PROTOTYPE_METHOD(t, "getItemCount", GetItemCount);
  NODE_SET_PROTOTYPE_METHOD(t, "getCommandIdAt", GetCommandIdAt);
  NODE_SET_PROTOTYPE_METHOD(t, "getLabelAt", GetLabelAt);
  NODE_SET_PROTOTYPE_METHOD(t, "getSublabelAt", GetSublabelAt);
  NODE_SET_PROTOTYPE_METHOD(t, "isItemCheckedAt", IsItemCheckedAt);
  NODE_SET_PROTOTYPE_METHOD(t, "isEnabledAt", IsEnabledAt);
  NODE_SET_PROTOTYPE_METHOD(t, "isVisibleAt", IsVisibleAt);

  NODE_SET_PROTOTYPE_METHOD(t, "popup", Popup);

  target->Set(v8::String::NewSymbol("Menu"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_menu, atom::api::Menu::Initialize)

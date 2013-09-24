// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_menu.h"

#include "browser/ui/accelerator_util.h"
#include "common/v8_conversions.h"

#define UNWRAP_MEMNU_AND_CHECK \
  Menu* self = ObjectWrap::Unwrap<Menu>(args.This()); \
  if (self == NULL) \
    return node::ThrowError("Menu is already destroyed")

namespace atom {

namespace api {

namespace {

// Call method of delegate object.
v8::Handle<v8::Value> CallDelegate(v8::Handle<v8::Value> default_value,
                                   v8::Handle<v8::Object> menu,
                                   const char* method,
                                   int command_id) {
  v8::HandleScope scope;

  v8::Handle<v8::Value> delegate = menu->Get(v8::String::New("delegate"));
  if (!delegate->IsObject())
    return default_value;

  v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(
      delegate->ToObject()->Get(v8::String::New(method)));
  if (!function->IsFunction())
    return default_value;

  v8::Handle<v8::Value> argv = v8::Integer::New(command_id);

  return scope.Close(
      function->Call(v8::Context::GetCurrent()->Global(), 1, &argv));
}

}  // namespace

Menu::Menu(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper),
      model_(new ui::SimpleMenuModel(this)) {
}

Menu::~Menu() {
}

bool Menu::IsCommandIdChecked(int command_id) const {
  v8::HandleScope scope;
  return CallDelegate(v8::False(),
                      handle(),
                      "isCommandIdChecked",
                      command_id)->BooleanValue();
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  v8::HandleScope scope;
  return CallDelegate(v8::True(),
                      handle(),
                      "isCommandIdEnabled",
                      command_id)->BooleanValue();
}

bool Menu::IsCommandIdVisible(int command_id) const {
  v8::HandleScope scope;
  return CallDelegate(v8::True(),
                      handle(),
                      "isCommandIdVisible",
                      command_id)->BooleanValue();
}

bool Menu::GetAcceleratorForCommandId(int command_id,
                                      ui::Accelerator* accelerator) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> shortcut = CallDelegate(v8::Undefined(),
                                                handle(),
                                                "getAcceleratorForCommandId",
                                                command_id);
  if (shortcut->IsString()) {
    std::string shortcut_str = FromV8Value(shortcut);
    return accelerator_util::StringToAccelerator(shortcut_str, accelerator);
  }

  return false;
}

bool Menu::IsItemForCommandIdDynamic(int command_id) const {
  v8::HandleScope scope;
  return CallDelegate(v8::False(),
                      handle(),
                      "isItemForCommandIdDynamic",
                      command_id)->BooleanValue();
}

string16 Menu::GetLabelForCommandId(int command_id) const {
  v8::HandleScope scope;
  return FromV8Value(CallDelegate(v8::False(),
                                  handle(),
                                  "getLabelForCommandId",
                                  command_id));
}

string16 Menu::GetSublabelForCommandId(int command_id) const {
  v8::HandleScope scope;
  return FromV8Value(CallDelegate(v8::False(),
                                  handle(),
                                  "getSubLabelForCommandId",
                                  command_id));
}

void Menu::ExecuteCommand(int command_id, int event_flags) {
  v8::HandleScope scope;
  CallDelegate(v8::False(), handle(), "executeCommand", command_id);
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

  int index, command_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddItem(command_id, label);
  else
    self->model_->InsertItemAt(index, command_id, label);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertCheckItem(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index, command_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddCheckItem(command_id, label);
  else
    self->model_->InsertCheckItemAt(index, command_id, label);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertRadioItem(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index, command_id, group_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label, &group_id))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddRadioItem(command_id, label, group_id);
  else
    self->model_->InsertRadioItemAt(index, command_id, label, group_id);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertSeparator(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index;
  if (!FromV8Arguments(args, &index))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddSeparator(ui::NORMAL_SEPARATOR);
  else
    self->model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::InsertSubMenu(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index, command_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label))
    return node::ThrowTypeError("Bad argument");

  Menu* submenu = ObjectWrap::Unwrap<Menu>(args[3]->ToObject());
  if (!submenu)
    return node::ThrowTypeError("The submenu is already destroyed");

  if (index < 0)
    self->model_->AddSubMenu(command_id, label, submenu->model_.get());
  else
    self->model_->InsertSubMenuAt(
        index, command_id, label, submenu->model_.get());

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::SetIcon(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index;
  base::FilePath path;
  if (!FromV8Arguments(args, &index, &path))
    return node::ThrowTypeError("Bad argument");

  // FIXME use webkit_glue's image decoder here.

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Menu::SetSublabel(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index;
  string16 label;
  if (!FromV8Arguments(args, &index, &label))
    return node::ThrowTypeError("Bad argument");

  self->model_->SetSublabel(index, label);

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
  return ToV8Value(self->model_->GetLabelAt(index));
}

// static
v8::Handle<v8::Value> Menu::GetSublabelAt(const v8::Arguments &args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = args[0]->IntegerValue();
  return ToV8Value(self->model_->GetSublabelAt(index));
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

  atom::NativeWindow* window;
  if (!FromV8Arguments(args, &window))
    return node::ThrowTypeError("Bad argument");

  self->Popup(window);
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

#if defined(OS_MACOSX)
  NODE_SET_METHOD(target, "setApplicationMenu", SetApplicationMenu);
  NODE_SET_METHOD(
      target, "sendActionToFirstResponder", SendActionToFirstResponder);
#endif
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_menu, atom::api::Menu::Initialize)

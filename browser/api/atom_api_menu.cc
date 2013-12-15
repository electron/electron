// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_menu.h"

#include "browser/ui/accelerator_util.h"
#include "common/v8/node_common.h"
#include "common/v8/native_type_conversions.h"

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
  v8::HandleScope handle_scope(node_isolate);

  v8::Handle<v8::Value> delegate = menu->Get(v8::String::New("delegate"));
  if (!delegate->IsObject())
    return default_value;

  v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(
      delegate->ToObject()->Get(v8::String::New(method)));
  if (!function->IsFunction())
    return default_value;

  v8::Handle<v8::Value> argv = v8::Integer::New(command_id);

  return handle_scope.Close(
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
  v8::HandleScope handle_scope(node_isolate);
  return CallDelegate(v8::False(),
                      const_cast<Menu*>(this)->handle(),
                      "isCommandIdChecked",
                      command_id)->BooleanValue();
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  v8::HandleScope handle_scope(node_isolate);
  return CallDelegate(v8::True(),
                      const_cast<Menu*>(this)->handle(),
                      "isCommandIdEnabled",
                      command_id)->BooleanValue();
}

bool Menu::IsCommandIdVisible(int command_id) const {
  v8::HandleScope handle_scope(node_isolate);
  return CallDelegate(v8::True(),
                      const_cast<Menu*>(this)->handle(),
                      "isCommandIdVisible",
                      command_id)->BooleanValue();
}

bool Menu::GetAcceleratorForCommandId(int command_id,
                                      ui::Accelerator* accelerator) {
  v8::HandleScope handle_scope(node_isolate);
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
  v8::HandleScope handle_scope(node_isolate);
  return CallDelegate(v8::False(),
                      const_cast<Menu*>(this)->handle(),
                      "isItemForCommandIdDynamic",
                      command_id)->BooleanValue();
}

string16 Menu::GetLabelForCommandId(int command_id) const {
  v8::HandleScope handle_scope(node_isolate);
  return FromV8Value(CallDelegate(v8::False(),
                                  const_cast<Menu*>(this)->handle(),
                                  "getLabelForCommandId",
                                  command_id));
}

string16 Menu::GetSublabelForCommandId(int command_id) const {
  v8::HandleScope handle_scope(node_isolate);
  return FromV8Value(CallDelegate(v8::False(),
                                  const_cast<Menu*>(this)->handle(),
                                  "getSubLabelForCommandId",
                                  command_id));
}

void Menu::ExecuteCommand(int command_id, int event_flags) {
  v8::HandleScope handle_scope(node_isolate);
  CallDelegate(v8::False(), handle(), "executeCommand", command_id);
}

// static
void Menu::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope handle_scope(args.GetIsolate());

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  Menu::Create(args.This());
}

// static
void Menu::InsertItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index, command_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddItem(command_id, label);
  else
    self->model_->InsertItemAt(index, command_id, label);
}

// static
void Menu::InsertCheckItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index, command_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddCheckItem(command_id, label);
  else
    self->model_->InsertCheckItemAt(index, command_id, label);
}

// static
void Menu::InsertRadioItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index, command_id, group_id;
  string16 label;
  if (!FromV8Arguments(args, &index, &command_id, &label, &group_id))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddRadioItem(command_id, label, group_id);
  else
    self->model_->InsertRadioItemAt(index, command_id, label, group_id);
}

// static
void Menu::InsertSeparator(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index;
  if (!FromV8Arguments(args, &index))
    return node::ThrowTypeError("Bad argument");

  if (index < 0)
    self->model_->AddSeparator(ui::NORMAL_SEPARATOR);
  else
    self->model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
}

// static
void Menu::InsertSubMenu(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
}

// static
void Menu::SetIcon(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index;
  base::FilePath path;
  if (!FromV8Arguments(args, &index, &path))
    return node::ThrowTypeError("Bad argument");

  // FIXME use webkit_glue's image decoder here.
}

// static
void Menu::SetSublabel(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  int index;
  string16 label;
  if (!FromV8Arguments(args, &index, &label))
    return node::ThrowTypeError("Bad argument");

  self->model_->SetSublabel(index, label);
}

// static
void Menu::Clear(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  self->model_->Clear();
}

// static
void Menu::GetIndexOfCommandId(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(self->model_->GetIndexOfCommandId(index));
}

// static
void Menu::GetItemCount(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  args.GetReturnValue().Set(self->model_->GetItemCount());
}

// static
void Menu::GetCommandIdAt(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(self->model_->GetCommandIdAt(index));
}

// static
void Menu::GetLabelAt(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(ToV8Value(self->model_->GetLabelAt(index)));
}

// static
void Menu::GetSublabelAt(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(ToV8Value(self->model_->GetSublabelAt(index)));
}

// static
void Menu::IsItemCheckedAt(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(self->model_->IsItemCheckedAt(index));
}

// static
void Menu::IsEnabledAt(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(self->model_->IsEnabledAt(index));
}

// static
void Menu::IsVisibleAt(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;
  int index = FromV8Value(args[0]);
  args.GetReturnValue().Set(self->model_->IsVisibleAt(index));
}

// static
void Menu::Popup(const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_MEMNU_AND_CHECK;

  atom::NativeWindow* window;
  if (!FromV8Arguments(args, &window))
    return node::ThrowTypeError("Bad argument");

  self->Popup(window);
}

// static
void Menu::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(node_isolate);

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

#if defined(OS_WIN)
  NODE_SET_PROTOTYPE_METHOD(t, "attachToWindow", AttachToWindow);
#endif

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

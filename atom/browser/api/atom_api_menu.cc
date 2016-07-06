// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_menu.h"

#include "atom/browser/native_window.h"
#include "atom/common/native_mate_converters/accelerator_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

Menu::Menu(v8::Isolate* isolate)
    : model_(new AtomMenuModel(this)),
      parent_(NULL) {
}

Menu::~Menu() {
}

void Menu::AfterInit(v8::Isolate* isolate) {
  mate::Dictionary wrappable(isolate, GetWrapper());
  mate::Dictionary delegate;
  if (!wrappable.Get("delegate", &delegate))
    return;

  delegate.Get("isCommandIdChecked", &is_checked_);
  delegate.Get("isCommandIdEnabled", &is_enabled_);
  delegate.Get("isCommandIdVisible", &is_visible_);
  delegate.Get("getAcceleratorForCommandId", &get_accelerator_);
  delegate.Get("executeCommand", &execute_command_);
  delegate.Get("menuWillShow", &menu_will_show_);
}

bool Menu::IsCommandIdChecked(int command_id) const {
  return is_checked_.Run(command_id);
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  return is_enabled_.Run(command_id);
}

bool Menu::IsCommandIdVisible(int command_id) const {
  return is_visible_.Run(command_id);
}

bool Menu::GetAcceleratorForCommandId(int command_id,
                                      ui::Accelerator* accelerator) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Value> val = get_accelerator_.Run(command_id);
  return mate::ConvertFromV8(isolate(), val, accelerator);
}

void Menu::ExecuteCommand(int command_id, int flags) {
  execute_command_.Run(
      mate::internal::CreateEventFromFlags(isolate(), flags),
      command_id);
}

void Menu::MenuWillShow(ui::SimpleMenuModel* source) {
  menu_will_show_.Run();
}

void Menu::InsertItemAt(
    int index, int command_id, const base::string16& label) {
  model_->InsertItemAt(index, command_id, label);
}

void Menu::InsertSeparatorAt(int index) {
  model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
}

void Menu::InsertCheckItemAt(int index,
                             int command_id,
                             const base::string16& label) {
  model_->InsertCheckItemAt(index, command_id, label);
}

void Menu::InsertRadioItemAt(int index,
                             int command_id,
                             const base::string16& label,
                             int group_id) {
  model_->InsertRadioItemAt(index, command_id, label, group_id);
}

void Menu::InsertSubMenuAt(int index,
                           int command_id,
                           const base::string16& label,
                           Menu* menu) {
  menu->parent_ = this;
  model_->InsertSubMenuAt(index, command_id, label, menu->model_.get());
}

void Menu::SetIcon(int index, const gfx::Image& image) {
  model_->SetIcon(index, image);
}

void Menu::SetSublabel(int index, const base::string16& sublabel) {
  model_->SetSublabel(index, sublabel);
}

void Menu::SetRole(int index, const base::string16& role) {
  model_->SetRole(index, role);
}

void Menu::Clear() {
  model_->Clear();
}

int Menu::GetIndexOfCommandId(int command_id) {
  return model_->GetIndexOfCommandId(command_id);
}

int Menu::GetItemCount() const {
  return model_->GetItemCount();
}

int Menu::GetCommandIdAt(int index) const {
  return model_->GetCommandIdAt(index);
}

base::string16 Menu::GetLabelAt(int index) const {
  return model_->GetLabelAt(index);
}

base::string16 Menu::GetSublabelAt(int index) const {
  return model_->GetSublabelAt(index);
}

bool Menu::IsItemCheckedAt(int index) const {
  return model_->IsItemCheckedAt(index);
}

bool Menu::IsEnabledAt(int index) const {
  return model_->IsEnabledAt(index);
}

bool Menu::IsVisibleAt(int index) const {
  return model_->IsVisibleAt(index);
}

// static
void Menu::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .MakeDestroyable()
      .SetMethod("insertItem", &Menu::InsertItemAt)
      .SetMethod("insertCheckItem", &Menu::InsertCheckItemAt)
      .SetMethod("insertRadioItem", &Menu::InsertRadioItemAt)
      .SetMethod("insertSeparator", &Menu::InsertSeparatorAt)
      .SetMethod("insertSubMenu", &Menu::InsertSubMenuAt)
      .SetMethod("setIcon", &Menu::SetIcon)
      .SetMethod("setSublabel", &Menu::SetSublabel)
      .SetMethod("setRole", &Menu::SetRole)
      .SetMethod("clear", &Menu::Clear)
      .SetMethod("getIndexOfCommandId", &Menu::GetIndexOfCommandId)
      .SetMethod("getItemCount", &Menu::GetItemCount)
      .SetMethod("getCommandIdAt", &Menu::GetCommandIdAt)
      .SetMethod("getLabelAt", &Menu::GetLabelAt)
      .SetMethod("getSublabelAt", &Menu::GetSublabelAt)
      .SetMethod("isItemCheckedAt", &Menu::IsItemCheckedAt)
      .SetMethod("isEnabledAt", &Menu::IsEnabledAt)
      .SetMethod("isVisibleAt", &Menu::IsVisibleAt)
      .SetMethod("popupAt", &Menu::PopupAt);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  using atom::api::Menu;
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<Menu>(
      isolate, "Menu", base::Bind(&Menu::Create));
  mate::Dictionary dict(isolate, exports);
  dict.Set("Menu", static_cast<v8::Local<v8::Value>>(constructor));
#if defined(OS_MACOSX)
  dict.SetMethod("setApplicationMenu", &Menu::SetApplicationMenu);
  dict.SetMethod("sendActionToFirstResponder",
                 &Menu::SendActionToFirstResponder);
#endif
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_menu, Initialize)

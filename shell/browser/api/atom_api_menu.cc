// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_menu.h"

#include <map>
#include <utility>

#include "shell/browser/native_window.h"
#include "shell/common/gin_converters/accelerator_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace {

// We need this map to keep references to currently opened menus.
// Without this menus would be destroyed by js garbage collector
// even when they are still displayed.
std::map<uint32_t, v8::Global<v8::Object>> g_menus;

}  // unnamed namespace

namespace electron {

namespace api {

Menu::Menu(gin::Arguments* args) : model_(new AtomMenuModel(this)) {
  InitWithArgs(args);
  model_->AddObserver(this);
}

Menu::~Menu() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

void Menu::AfterInit(v8::Isolate* isolate) {
  gin::Dictionary wrappable(isolate, GetWrapper());
  gin::Dictionary delegate(nullptr);
  if (!wrappable.Get("delegate", &delegate))
    return;

  delegate.Get("isCommandIdChecked", &is_checked_);
  delegate.Get("isCommandIdEnabled", &is_enabled_);
  delegate.Get("isCommandIdVisible", &is_visible_);
  delegate.Get("shouldCommandIdWorkWhenHidden", &works_when_hidden_);
  delegate.Get("getAcceleratorForCommandId", &get_accelerator_);
  delegate.Get("shouldRegisterAcceleratorForCommandId",
               &should_register_accelerator_);
  delegate.Get("executeCommand", &execute_command_);
  delegate.Get("menuWillShow", &menu_will_show_);
}

bool Menu::IsCommandIdChecked(int command_id) const {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  return is_checked_.Run(GetWrapper(), command_id);
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  return is_enabled_.Run(GetWrapper(), command_id);
}

bool Menu::IsCommandIdVisible(int command_id) const {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  return is_visible_.Run(GetWrapper(), command_id);
}

bool Menu::ShouldCommandIdWorkWhenHidden(int command_id) const {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  return works_when_hidden_.Run(GetWrapper(), command_id);
}

bool Menu::GetAcceleratorForCommandIdWithParams(
    int command_id,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Value> val =
      get_accelerator_.Run(GetWrapper(), command_id, use_default_accelerator);
  return gin::ConvertFromV8(isolate(), val, accelerator);
}

bool Menu::ShouldRegisterAcceleratorForCommandId(int command_id) const {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  return should_register_accelerator_.Run(GetWrapper(), command_id);
}

void Menu::ExecuteCommand(int command_id, int flags) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  execute_command_.Run(
      GetWrapper(),
      gin_helper::internal::CreateEventFromFlags(isolate(), flags), command_id);
}

void Menu::OnMenuWillShow(ui::SimpleMenuModel* source) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  menu_will_show_.Run(GetWrapper());
}

void Menu::InsertItemAt(int index,
                        int command_id,
                        const base::string16& label) {
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

void Menu::SetToolTip(int index, const base::string16& toolTip) {
  model_->SetToolTip(index, toolTip);
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

base::string16 Menu::GetToolTipAt(int index) const {
  return model_->GetToolTipAt(index);
}

base::string16 Menu::GetAcceleratorTextAt(int index) const {
  ui::Accelerator accelerator;
  model_->GetAcceleratorAtWithParams(index, true, &accelerator);
  return accelerator.GetShortcutText();
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

bool Menu::WorksWhenHiddenAt(int index) const {
  return model_->WorksWhenHiddenAt(index);
}

void Menu::OnMenuWillClose() {
  g_menus.erase(weak_map_id());
  Emit("menu-will-close");
}

void Menu::OnMenuWillShow() {
  g_menus[weak_map_id()] = v8::Global<v8::Object>(isolate(), GetWrapper());
  Emit("menu-will-show");
}

base::OnceClosure Menu::BindSelfToClosure(base::OnceClosure callback) {
  // return ((callback, ref) => { callback() }).bind(null, callback, this)
  v8::Global<v8::Value> ref(isolate(), GetWrapper());
  return base::BindOnce(
      [](base::OnceClosure callback, v8::Global<v8::Value> ref) {
        std::move(callback).Run();
      },
      std::move(callback), std::move(ref));
}

// static
void Menu::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "Menu"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("insertItem", &Menu::InsertItemAt)
      .SetMethod("insertCheckItem", &Menu::InsertCheckItemAt)
      .SetMethod("insertRadioItem", &Menu::InsertRadioItemAt)
      .SetMethod("insertSeparator", &Menu::InsertSeparatorAt)
      .SetMethod("insertSubMenu", &Menu::InsertSubMenuAt)
      .SetMethod("setIcon", &Menu::SetIcon)
      .SetMethod("setSublabel", &Menu::SetSublabel)
      .SetMethod("setToolTip", &Menu::SetToolTip)
      .SetMethod("setRole", &Menu::SetRole)
      .SetMethod("clear", &Menu::Clear)
      .SetMethod("getIndexOfCommandId", &Menu::GetIndexOfCommandId)
      .SetMethod("getItemCount", &Menu::GetItemCount)
      .SetMethod("getCommandIdAt", &Menu::GetCommandIdAt)
      .SetMethod("getLabelAt", &Menu::GetLabelAt)
      .SetMethod("getSublabelAt", &Menu::GetSublabelAt)
      .SetMethod("getToolTipAt", &Menu::GetToolTipAt)
      .SetMethod("getAcceleratorTextAt", &Menu::GetAcceleratorTextAt)
      .SetMethod("isItemCheckedAt", &Menu::IsItemCheckedAt)
      .SetMethod("isEnabledAt", &Menu::IsEnabledAt)
      .SetMethod("worksWhenHiddenAt", &Menu::WorksWhenHiddenAt)
      .SetMethod("isVisibleAt", &Menu::IsVisibleAt)
      .SetMethod("popupAt", &Menu::PopupAt)
      .SetMethod("closePopupAt", &Menu::ClosePopupAt);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::Menu;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  Menu::SetConstructor(isolate, base::BindRepeating(&Menu::New));

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set(
      "Menu",
      Menu::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());
#if defined(OS_MACOSX)
  dict.SetMethod("setApplicationMenu", &Menu::SetApplicationMenu);
  dict.SetMethod("sendActionToFirstResponder",
                 &Menu::SendActionToFirstResponder);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_menu, Initialize)

// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_menu.h"

#include <string>
#include <string_view>
#include <utility>

#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/api/ui_event.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/gin_converters/accelerator_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/optional_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "ui/base/models/image_model.h"
#include "v8/include/cppgc/persistent.h"

#if BUILDFLAG(IS_MAC)

namespace gin {

using SharingItem = electron::ElectronMenuModel::SharingItem;
using Badge = electron::ElectronMenuModel::Badge;

template <>
struct Converter<SharingItem> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     SharingItem* out) {
    gin_helper::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.GetOptional("texts", &(out->texts));
    dict.GetOptional("filePaths", &(out->file_paths));
    dict.GetOptional("urls", &(out->urls));
    return true;
  }
};

template <>
struct Converter<Badge> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     Badge* out) {
    gin_helper::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    out->type = "none";
    dict.Get("type", &(out->type));
    dict.GetOptional("count", &(out->count));
    dict.GetOptional("content", &(out->content));
    return true;
  }
};

}  // namespace gin

#endif

namespace electron::api {

const gin::WrapperInfo Menu::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronMenu);

Menu::Menu(gin::Arguments* args)
    : model_(std::make_unique<ElectronMenuModel>(this)) {
  model_->AddObserver(this);

#if BUILDFLAG(IS_MAC)
  gin_helper::Dictionary options;
  if (args->GetNext(&options)) {
    ElectronMenuModel::SharingItem item;
    if (options.Get("sharingItem", &item))
      model_->SetSharingItem(std::move(item));
  }
#endif
}

Menu::~Menu() {
  RemoveModelObserver();
}

void Menu::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<Menu>::Trace(visitor);
  visitor->Trace(parent_);
}

void Menu::RemoveModelObserver() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

namespace {

v8::Local<v8::Value> Get(v8::Local<v8::Context> context,
                         v8::Local<v8::Object> object,
                         std::string_view key) {
  v8::Local<v8::Value> value;
  if (object
          ->Get(context,
                gin::StringToSymbol(JavascriptEnvironment::GetIsolate(), key))
          .ToLocal(&value)) {
    return value;
  }
  return v8::Undefined(JavascriptEnvironment::GetIsolate());
}

// The window the built-in window roles act on: the focused one, as
// BaseWindow.getFocusedWindow() would report it.
NativeWindow* FocusedWindow() {
  for (NativeWindow* window : WindowList::GetWindows()) {
    if (!window->IsClosed() && window->IsFocused())
      return window;
  }
  return nullptr;
}

}  // namespace

// this.commandsMap[commandId]. The caller holds a HandleScope.
v8::MaybeLocal<v8::Object> Menu::GetItem(v8::Isolate* isolate,
                                         int command_id) const {
  v8::Local<v8::Object> wrapper;
  if (!const_cast<Menu*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
    return {};
  v8::Local<v8::Context> context = wrapper->GetCreationContextChecked(isolate);
  v8::Local<v8::Value> commands = Get(context, wrapper, "commandsMap");
  if (!commands->IsObject())
    return {};
  v8::Local<v8::Value> item;
  if (commands.As<v8::Object>()
          ->Get(context, v8::Integer::New(isolate, command_id))
          .ToLocal(&item) &&
      item->IsObject()) {
    return item.As<v8::Object>();
  }
  return {};
}

// this.commandsMap[commandId]?.[key]. The caller holds a HandleScope.
v8::Local<v8::Value> Menu::GetItemProperty(v8::Isolate* isolate,
                                           int command_id,
                                           std::string_view key) const {
  v8::Local<v8::Object> item;
  if (!GetItem(isolate, command_id).ToLocal(&item))
    return v8::Undefined(isolate);
  return Get(item->GetCreationContextChecked(isolate), item, key);
}

// Conversions match what the old callbacks did through gin: a flag by
// truthiness (a missing item or property reads as false), a string only if it
// is one.
bool Menu::GetItemFlag(int command_id, std::string_view key) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  return GetItemProperty(isolate, command_id, key)->BooleanValue(isolate);
}

std::u16string Menu::GetItemText(int command_id, std::string_view key) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> value = GetItemProperty(isolate, command_id, key);
  std::u16string text;
  if (value->IsString())
    gin::ConvertFromV8(isolate, value, &text);
  return text;
}

bool Menu::IsCommandIdChecked(int command_id) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> item;
  if (!GetItem(isolate, command_id).ToLocal(&item))
    return false;
  v8::Local<v8::Context> context = item->GetCreationContextChecked(isolate);
  if (!Get(context, item, "_dynamicChecked")->BooleanValue(isolate))
    return Get(context, item, "checked")->BooleanValue(isolate);
  // e.g. toggleSpellChecker: the role computes it.
  v8::Local<v8::Value> val = gin_helper::CallMethod(
      isolate, const_cast<Menu*>(this), "_isCommandIdChecked", command_id);
  bool checked = false;
  return gin::ConvertFromV8(isolate, val, &checked) && checked;
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> item;
  if (!GetItem(isolate, command_id).ToLocal(&item))
    return false;
  v8::Local<v8::Context> context = item->GetCreationContextChecked(isolate);
  // The window roles follow the focused window's abilities.
  std::string role;
  gin::ConvertFromV8(isolate, Get(context, item, "role"), &role);
  if (role == "minimize" || role == "togglefullscreen" || role == "close") {
    if (NativeWindow* window = FocusedWindow()) {
      if (role == "minimize")
        return window->IsMinimizable();
      if (role == "togglefullscreen")
        return window->IsFullScreenable();
      return window->IsClosable();
    }
  }
  return Get(context, item, "enabled")->BooleanValue(isolate);
}

std::u16string Menu::GetLabelForCommandId(int command_id) const {
  return GetItemText(command_id, "label");
}

std::u16string Menu::GetAccessibilityLabelForCommandId(int command_id) const {
  return GetItemText(command_id, "accessibilityLabel");
}

std::u16string Menu::GetSecondaryLabelForCommandId(int command_id) const {
  return GetItemText(command_id, "sublabel");
}

ui::ImageModel Menu::GetIconForCommandId(int command_id) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> icon = GetItemProperty(isolate, command_id, "icon");
  gfx::Image image;
  if (!icon->IsNullOrUndefined() && gin::ConvertFromV8(isolate, icon, &image))
    return ui::ImageModel::FromImage(image);
  return ui::ImageModel();
}

bool Menu::IsCommandIdVisible(int command_id) const {
  return GetItemFlag(command_id, "visible");
}

bool Menu::ShouldCommandIdWorkWhenHidden(int command_id) const {
  return GetItemFlag(command_id, "acceleratorWorksWhenHidden");
}

bool Menu::GetAcceleratorForCommandIdWithParams(
    int command_id,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  // MenuItem.accelerator already holds the role's default when the app gave
  // none, so |use_default_accelerator| has nothing further to add; a string
  // that does not parse means no accelerator, as before.
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> value =
      GetItemProperty(isolate, command_id, "accelerator");
  return value->IsString() && gin::ConvertFromV8(isolate, value, accelerator);
}

bool Menu::ShouldRegisterAcceleratorForCommandId(int command_id) const {
  return GetItemFlag(command_id, "registerAccelerator");
}

#if BUILDFLAG(IS_MAC)
bool Menu::GetSharingItemForCommandId(
    int command_id,
    ElectronMenuModel::SharingItem* item) const {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> value =
      GetItemProperty(isolate, command_id, "sharingItem");
  return !value->IsNullOrUndefined() &&
         gin::ConvertFromV8(isolate, value, item);
}
#endif

void Menu::ExecuteCommand(int command_id, int flags) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::CallMethod(isolate, const_cast<Menu*>(this), "_executeCommand",
                         CreateEventFromFlags(flags), command_id);
}

void Menu::OnMenuWillShow(ui::SimpleMenuModel* source) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin_helper::CallMethod(isolate, const_cast<Menu*>(this), "_menuWillShow");
}

base::OnceClosure Menu::BindSelfToClosure(base::OnceClosure callback) {
  return base::BindOnce(
      [](base::OnceClosure callback, cppgc::Persistent<Menu> prevent_gc) {
        std::move(callback).Run();
      },
      std::move(callback), cppgc::Persistent<Menu>(this));
}

void Menu::InsertItemAt(int index,
                        int command_id,
                        const std::u16string& label) {
  model_->InsertItemAt(index, command_id, label);
}

void Menu::InsertSeparatorAt(int index) {
  model_->InsertSeparatorAt(index, ui::NORMAL_SEPARATOR);
}

void Menu::InsertCheckItemAt(int index,
                             int command_id,
                             const std::u16string& label) {
  model_->InsertCheckItemAt(index, command_id, label);
}

void Menu::InsertRadioItemAt(int index,
                             int command_id,
                             const std::u16string& label,
                             int group_id) {
  model_->InsertRadioItemAt(index, command_id, label, group_id);
}

void Menu::InsertSubMenuAt(int index,
                           int command_id,
                           const std::u16string& label,
                           Menu* menu) {
  menu->parent_ = this;
  model_->InsertSubMenuAt(index, command_id, label, menu->model_.get());
}

void Menu::SetIcon(int index, const gfx::Image& image) {
  model_->SetIcon(index, ui::ImageModel::FromImage(image));
}

void Menu::SetToolTip(int index, const std::u16string& toolTip) {
  model_->SetToolTip(index, toolTip);
}

void Menu::SetRole(int index, const std::u16string& role) {
  model_->SetRole(index, role);
}

void Menu::SetCustomType(int index, const std::u16string& customType) {
  model_->SetCustomType(index, customType);
}

#if BUILDFLAG(IS_MAC)
void Menu::SetBadge(int index, std::optional<ElectronMenuModel::Badge> badge) {
  model_->SetBadge(index, std::move(badge));
}
#endif

void Menu::Clear() {
  model_->Clear();
}

int Menu::GetIndexOfCommandId(int command_id) const {
  return model_->GetIndexOfCommandId(command_id).value_or(-1);
}

int Menu::GetItemCount() const {
  return model_->GetItemCount();
}

std::u16string Menu::GetAcceleratorTextAtForTesting(int index) const {
  ui::Accelerator accelerator;
  model_->GetAcceleratorAtWithParams(index, true, &accelerator);
  return accelerator.GetShortcutText();
}

void Menu::OnMenuWillClose() {
  Emit("menu-will-close");
  keep_alive_.Clear();
}

void Menu::OnMenuWillShow() {
  keep_alive_ = this;
  Emit("menu-will-show");
}

// static
void Menu::FillObjectTemplate(v8::Isolate* isolate,
                              v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, "Menu", templ)
      .SetMethod("insertItem", &Menu::InsertItemAt)
      .SetMethod("insertCheckItem", &Menu::InsertCheckItemAt)
      .SetMethod("insertRadioItem", &Menu::InsertRadioItemAt)
      .SetMethod("insertSeparator", &Menu::InsertSeparatorAt)
      .SetMethod("insertSubMenu", &Menu::InsertSubMenuAt)
      .SetMethod("setIcon", &Menu::SetIcon)
      .SetMethod("setToolTip", &Menu::SetToolTip)
      .SetMethod("setRole", &Menu::SetRole)
      .SetMethod("setCustomType", &Menu::SetCustomType)
#if BUILDFLAG(IS_MAC)
      .SetMethod("setBadge", &Menu::SetBadge)
#endif
      .SetMethod("clear", &Menu::Clear)
      .SetMethod("getItemCount", &Menu::GetItemCount)
      .SetMethod("getIndexOfCommandId", &Menu::GetIndexOfCommandId)
      .SetMethod("popupAt", &Menu::PopupAt)
      .SetMethod("closePopupAt", &Menu::ClosePopupAt)
      .SetMethod("_getAcceleratorTextAt", &Menu::GetAcceleratorTextAtForTesting)
#if BUILDFLAG(IS_MAC)
      .SetMethod("_getUserAcceleratorAt", &Menu::GetUserAcceleratorAt)
      .SetMethod("_simulateSubmenuCloseSequenceForTesting",
                 &Menu::SimulateSubmenuCloseSequenceForTesting)
#endif
      .Build();
}

const gin::WrapperInfo* Menu::wrapper_info() const {
  return &kWrapperInfo;
}

const char* Menu::GetHumanReadableName() const {
  return "Electron / Menu";
}

}  // namespace electron::api

namespace {

using electron::api::Menu;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("Menu", Menu::GetConstructor(isolate, context, &Menu::kWrapperInfo));
#if BUILDFLAG(IS_MAC)
  dict.SetMethod("setApplicationMenu", &Menu::SetApplicationMenu);
  dict.SetMethod("sendActionToFirstResponder",
                 &Menu::SendActionToFirstResponder);
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_menu, Initialize)

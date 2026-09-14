// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_menu.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_menu_item.h"
#include "shell/browser/api/electron_api_menu_roles.h"
#include "shell/browser/api/electron_api_web_contents.h"
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
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"

namespace electron::api {

const gin::WrapperInfo Menu::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronMenu);

Menu::Menu(gin::Arguments* args)
    : model_(std::make_unique<ElectronMenuModel>(this)) {
  model_->AddObserver(this);

#if BUILDFLAG(IS_MAC)
  gin_helper::Dictionary options;
  if (args && args->GetNext(&options)) {
    ElectronMenuModel::SharingItem item;
    if (options.Get("sharingItem", &item))
      model_->SetSharingItem(std::move(item));
  }
#endif
}

// static
Menu* Menu::New(gin::Arguments* args) {
  return Create(args->isolate(), args);
}

Menu::~Menu() {
  RemoveModelObserver();
}

void Menu::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<Menu>::Trace(visitor);
  visitor->Trace(parent_);
  visitor->Trace(first_entry_);
  visitor->Trace(last_entry_);
  visitor->Trace(items_);
}

void Menu::RemoveModelObserver() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

namespace {}  // namespace

Menu::Entry::Entry(MenuItem* item, Entry* next) : item(item), next(next) {}
Menu::Entry::~Entry() = default;

void Menu::Entry::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(item);
  visitor->Trace(next);
}

MenuItem* Menu::GetItem(int command_id) const {
  return MenuItem::FromCommandId(command_id);
}

void Menu::ForEachInRadioGroup(int group_id,
                               base::FunctionRef<void(MenuItem*)> fn) const {
  auto it = radio_groups_.find(group_id);
  if (it == radio_groups_.end())
    return;
  for (int command_id : it->second) {
    if (MenuItem* item = GetItem(command_id))
      fn(item);
  }
}

// Search between separators around |pos| for a radio item and return its
// group id, else start a new group.
int Menu::GenerateGroupId(int pos) {
  static int next_group_id = 0;
  if (pos > 0) {
    // The nearest radio item before |pos| with no separator in between.
    int found = 0;
    int index = 0;
    for (Entry* e = first_entry(); e && index < pos;
         e = e->next.Get(), ++index) {
      if (e->item->type() == MenuItem::Type::kRadio)
        found = e->item->group_id();
      else if (e->item->type() == MenuItem::Type::kSeparator)
        found = 0;
    }
    if (found)
      return found;
  } else {
    // The first radio item from |pos| on, before any separator.
    for (Entry* e = first_entry(); e; e = e->next.Get()) {
      if (e->item->type() == MenuItem::Type::kRadio)
        return e->item->group_id();
      if (e->item->type() == MenuItem::Type::kSeparator)
        break;
    }
  }
  return ++next_group_id;
}

void Menu::Insert(gin_helper::ErrorThrower thrower,
                  int pos,
                  v8::Local<v8::Value> item_value) {
  v8::Isolate* isolate = thrower.isolate();
  MenuItem* item = MenuItem::FromV8(isolate, item_value);
  if (!item) {
    thrower.ThrowTypeError("Invalid item");
    return;
  }
  if (pos < 0) {
    thrower.ThrowRangeError(base::StrCat(
        {"Position ", base::NumberToString(pos), " cannot be less than 0"}));
    return;
  }
  if (pos > GetItemCount()) {
    thrower.ThrowRangeError(
        base::StrCat({"Position ", base::NumberToString(pos),
                      " cannot be greater than the total MenuItem count"}));
    return;
  }
  InsertItem(isolate, pos, item, &thrower);
}

void Menu::Append(gin_helper::ErrorThrower thrower, v8::Local<v8::Value> item) {
  Insert(thrower, GetItemCount(), item);
}

void Menu::AppendItem(v8::Isolate* isolate, MenuItem* item) {
  InsertItem(isolate, GetItemCount(), item);
}

void Menu::InsertItem(v8::Isolate* isolate,
                      int pos,
                      MenuItem* item,
                      gin_helper::ErrorThrower* thrower) {
  const int count = GetItemCount();
  CHECK_GE(pos, 0);
  CHECK_LE(pos, count);
  if ((item->type() == MenuItem::Type::kSubmenu ||
       item->type() == MenuItem::Type::kPalette) &&
      !item->submenu()) {
    if (thrower)
      thrower->ThrowTypeError("Invalid submenu");
    return;
  }
  const int id = item->command_id();
  int group_id = 0;
  switch (item->type()) {
    case MenuItem::Type::kNormal:
    case MenuItem::Type::kHeader:
      model_->InsertItemAt(pos, id, item->label());
      break;
    case MenuItem::Type::kCheckbox:
      model_->InsertCheckItemAt(pos, id, item->label());
      break;
    case MenuItem::Type::kSeparator:
      model_->InsertSeparatorAt(pos, ui::NORMAL_SEPARATOR);
      break;
    case MenuItem::Type::kSubmenu:
    case MenuItem::Type::kPalette:
      item->submenu()->parent_ = this;
      model_->InsertSubMenuAt(pos, id, item->label(), item->submenu()->model());
      break;
    case MenuItem::Type::kRadio:
      group_id = item->group_id() ? item->group_id() : GenerateGroupId(pos);
      radio_groups_[group_id].push_back(id);
      model_->InsertRadioItemAt(pos, id, item->label(), group_id);
      break;
  }

  item->AttachToMenu(this, group_id);
  cppgc::AllocationHandle& heap = isolate->GetCppHeap()->GetAllocationHandle();
  if (pos == count) {
    Entry* entry = cppgc::MakeGarbageCollected<Entry>(heap, item, nullptr);
    if (last_entry_)
      last_entry_->next = entry;
    else
      first_entry_ = entry;
    last_entry_ = entry;
  } else if (pos == 0) {
    first_entry_ =
        cppgc::MakeGarbageCollected<Entry>(heap, item, first_entry());
  } else {
    Entry* before = first_entry();
    for (int i = 1; i < pos; ++i)
      before = before->next.Get();
    before->next =
        cppgc::MakeGarbageCollected<Entry>(heap, item, before->next.Get());
  }

  items_.Reset();

  if (!item->tool_tip().empty())
    model_->SetToolTip(pos, item->tool_tip());
  if (!item->role_name().empty())
    model_->SetRole(pos, base::UTF8ToUTF16(item->role_name()));
  if (item->type() == MenuItem::Type::kPalette)
    model_->SetCustomType(pos, u"palette");
  else if (item->type() == MenuItem::Type::kHeader)
    model_->SetCustomType(pos, u"header");
#if BUILDFLAG(IS_MAC)
  if (const auto* badge = item->badge())
    model_->SetBadge(pos, *badge);
#endif
}

#if BUILDFLAG(IS_MAC)
void Menu::UpdateBadge(MenuItem* item) {
  const int index = GetIndexOfCommandId(item->command_id());
  if (index != -1)
    model_->SetBadge(index, item->badge() ? std::make_optional(*item->badge())
                                          : std::nullopt);
}
#endif

v8::Local<v8::Value> Menu::Items(v8::Isolate* isolate) {
  if (items_.IsEmpty()) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Array> items = v8::Array::New(isolate);
    uint32_t i = 0;
    for (Entry* e = first_entry(); e; e = e->next.Get()) {
      v8::Local<v8::Object> wrapper;
      if (e->item->GetWrapper(isolate).ToLocal(&wrapper))
        items->Set(context, i++, wrapper).Check();
    }
    items_.Reset(isolate, items);
  }
  return items_.Get(isolate);
}

bool Menu::IsCommandIdChecked(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item && item->IsChecked();
}

bool Menu::IsCommandIdEnabled(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item && item->IsEnabled();
}

std::u16string Menu::GetLabelForCommandId(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item ? item->label() : std::u16string();
}

std::u16string Menu::GetAccessibilityLabelForCommandId(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item ? item->accessibility_label() : std::u16string();
}

std::u16string Menu::GetSecondaryLabelForCommandId(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item ? item->sublabel() : std::u16string();
}

ui::ImageModel Menu::GetIconForCommandId(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item ? item->icon() : ui::ImageModel();
}

bool Menu::IsCommandIdVisible(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item && item->visible();
}

bool Menu::ShouldCommandIdWorkWhenHidden(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item && item->works_when_hidden();
}

bool Menu::GetAcceleratorForCommandIdWithParams(
    int command_id,
    bool use_default_accelerator,
    ui::Accelerator* accelerator) const {
  MenuItem* item = GetItem(command_id);
  if (!item || !item->accelerator())
    return false;
  *accelerator = *item->accelerator();
  return true;
}

bool Menu::ShouldRegisterAcceleratorForCommandId(int command_id) const {
  MenuItem* item = GetItem(command_id);
  return item && item->register_accelerator();
}

#if BUILDFLAG(IS_MAC)
bool Menu::GetSharingItemForCommandId(
    int command_id,
    ElectronMenuModel::SharingItem* out) const {
  MenuItem* item = GetItem(command_id);
  const ElectronMenuModel::SharingItem* sharing_item =
      item ? item->GetSharingItem(JavascriptEnvironment::GetIsolate())
           : nullptr;
  if (!sharing_item)
    return false;
  *out = *sharing_item;
  return true;
}
#endif

void Menu::ExecuteCommand(int command_id, int flags) {
  MenuItem* item = GetItem(command_id);
  if (!item)
    return;
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;
  node::CallbackScope callback_scope(isolate, wrapper, {0, 0});
  item->Activate(BaseWindow::GetFocusedWindow(),
                 WebContents::GetFocusedWebContents(), flags);
}

void Menu::ActivateForTesting(int command_id) {
  if (MenuItem* item = GetItem(command_id)) {
    item->Activate(BaseWindow::GetFocusedWindow(),
                   WebContents::GetFocusedWebContents(), 0);
  }
}

void Menu::MenuWillShowForTesting() {
  OnMenuWillShow(model_.get());
}

void Menu::OnMenuWillShow(ui::SimpleMenuModel* source) {
  // Ensure radio groups have at least one item selected.
  for (const auto& [group_id, command_ids] : radio_groups_) {
    bool any_checked = false;
    MenuItem* first = nullptr;
    ForEachInRadioGroup(group_id, [&](MenuItem* item) {
      if (!first)
        first = item;
      any_checked = any_checked || item->checked_flag();
    });
    if (!any_checked && first)
      first->SetCheckedForRadioGroup();
  }
}

base::OnceClosure Menu::BindSelfToClosure(base::OnceClosure callback) {
  return base::BindOnce(
      [](base::OnceClosure callback, cppgc::Persistent<Menu> prevent_gc) {
        std::move(callback).Run();
      },
      std::move(callback), cppgc::Persistent<Menu>(this));
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
      .SetMethod("insert", &Menu::Insert)
      .SetMethod("append", &Menu::Append)
      .SetMethod("popupAt", &Menu::PopupAt)
      .SetMethod("closePopupAt", &Menu::ClosePopupAt)
      .SetMethod("getItemCount", &Menu::GetItemCount)
      .SetMethod("getIndexOfCommandId", &Menu::GetIndexOfCommandId)
      .SetMethod("_activate", &Menu::ActivateForTesting)
      .SetMethod("_menuWillShow", &Menu::MenuWillShowForTesting)
      .SetMethod("_getAcceleratorTextAt", &Menu::GetAcceleratorTextAtForTesting)
#if BUILDFLAG(IS_MAC)
      .SetMethod("_getUserAcceleratorAt", &Menu::GetUserAcceleratorAt)
      .SetMethod("_simulateSubmenuCloseSequenceForTesting",
                 &Menu::SimulateSubmenuCloseSequenceForTesting)
#endif
      .Build();
}

// static
void Menu::FillInstanceTemplate(v8::Isolate* isolate,
                                v8::Local<v8::ObjectTemplate> templ) {
  // An own, enumerable property so that a serialised menu includes it.
  templ->SetNativeDataProperty(
      gin::StringToSymbol(isolate, "items"),
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        Menu* self = nullptr;
        if (gin::ConvertFromV8(info.GetIsolate(), info.Holder(), &self) && self)
          info.GetReturnValue().Set(self->Items(info.GetIsolate()));
      },
      nullptr, v8::Local<v8::Value>(),
      static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
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
  v8::Local<v8::Function> menu =
      Menu::GetConstructor(isolate, context, &Menu::kWrapperInfo);
  dict.Set("Menu", menu);
  dict.Set("MenuItem",
           electron::api::MenuItem::GetConstructor(
               isolate, context, &electron::api::MenuItem::kWrapperInfo));
  gin_helper::Dictionary statics(isolate, menu);
  statics.SetMethod("buildFromTemplate", &Menu::BuildFromTemplate);
  statics.SetMethod("_roleDefaults", &electron::api::menu_roles::Defaults);
#if BUILDFLAG(IS_MAC)
  dict.SetMethod("setApplicationMenu", &Menu::SetApplicationMenu);
  statics.SetMethod("sendActionToFirstResponder",
                    &Menu::SendActionToFirstResponder);
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_menu, Initialize)

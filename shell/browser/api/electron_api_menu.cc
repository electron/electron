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

namespace {

cppgc::Persistent<Menu>& ApplicationMenu() {
  static base::NoDestructor<cppgc::Persistent<Menu>> menu;
  return *menu;
}
bool g_application_menu_was_set = false;

}  // namespace

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

v8::Local<v8::Value> Menu::GetMenuItemById(gin::Arguments* args) {
  v8::Local<v8::Value> id;
  if (!args->GetNext(&id))
    id = v8::Undefined(args->isolate());
  return FindItemById(args->isolate(), id);
}

v8::Local<v8::Value> Menu::FindItemById(v8::Isolate* isolate,
                                        v8::Local<v8::Value> id) {
  for (Entry* e = first_entry(); e; e = e->next.Get()) {
    v8::Local<v8::Object> wrapper;
    if (!e->item->GetWrapper(isolate).ToLocal(&wrapper))
      continue;
    v8::Local<v8::Value> item_id;
    if (!gin_helper::Dictionary(isolate, wrapper).Get("id", &item_id))
      item_id = v8::Undefined(isolate);
    if (item_id->StrictEquals(id))
      return wrapper;
  }
  for (Entry* e = first_entry(); e; e = e->next.Get()) {
    if (e->item->submenu()) {
      v8::Local<v8::Value> found =
          e->item->submenu()->FindItemById(isolate, id);
      if (!found->IsNull())
        return found;
    }
  }
  return v8::Null(isolate);
}

v8::Local<v8::Value> Menu::Popup(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);
  v8::Local<v8::Value> options_value;
  if (!args->GetNext(&options_value) || options_value->IsUndefined())
    options_value = v8::Object::New(isolate);
  if (!options_value->IsObject() || options_value->IsNull()) {
    thrower.ThrowTypeError("Options must be an object");
    return v8::Undefined(isolate);
  }
  v8::Local<v8::Object> options = options_value.As<v8::Object>();

  gin_helper::Dictionary dict(isolate, options);
  v8::Local<v8::Value> window_value;
  int x = -1;
  int y = -1;
  int positioning_item = -1;
  ui::mojom::MenuSourceType source_type = ui::mojom::MenuSourceType::kMouse;
  WebFrameMain* frame_ptr = nullptr;
  dict.Get("window", &window_value);
  // A number must be an int32; anything else takes the default.
  bool bad_number = false;
  auto get_int = [&](std::string_view key, int* out) {
    v8::Local<v8::Value> value;
    if (!dict.Get(key, &value) || !value->IsNumber())
      return;
    if (value->IsInt32())
      *out = value.As<v8::Int32>()->Value();
    else
      bad_number = true;
  };
  get_int("x", &x);
  get_int("y", &y);
  get_int("positioningItem", &positioning_item);
  if (bad_number) {
    thrower.ThrowTypeError("x, y and positioningItem must be integers");
    return v8::Undefined(isolate);
  }
  std::string source_type_name;
  if (dict.Get("sourceType", &source_type_name) && !source_type_name.empty() &&
      !gin::ConvertFromV8(isolate, gin::StringToV8(isolate, source_type_name),
                          &source_type)) {
    thrower.ThrowTypeError("Invalid sourceType: " + source_type_name);
    return v8::Undefined(isolate);
  }
  dict.Get("frame", &frame_ptr);

  BaseWindow* window = nullptr;
  if (!window_value.IsEmpty())
    window = BaseWindow::FromValue(isolate, window_value);
  if (!window) {
    window = BaseWindow::GetFocusedWindow();
    if (!window && !BaseWindow::GetAllNative().empty())
      window = BaseWindow::GetAllNative().front();
    if (!window) {
      thrower.ThrowError("Cannot open Menu without a BaseWindow present");
      return v8::Undefined(isolate);
    }
    window_value = window->GetWrapper();
  }
  std::optional<WebFrameMain*> frame;
  if (frame_ptr)
    frame = frame_ptr;
  // Anything but a function means no callback.
  base::OnceClosure callback;
  if (!dict.Get("callback", &callback))
    callback = base::DoNothing();
  PopupAt(window, frame, x, y, positioning_item, source_type,
          std::move(callback));

  gin::Dictionary result = gin::Dictionary::CreateEmpty(isolate);
  result.Set("browserWindow", window_value);
  result.Set("x", x);
  result.Set("y", y);
  result.Set("position", positioning_item);
  return gin::ConvertToV8(isolate, result);
}

void Menu::ClosePopup(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Value> window_value;
  BaseWindow* window = nullptr;
  if (args->GetNext(&window_value))
    window = BaseWindow::FromValue(isolate, window_value);
  if (window) {
    ClosePopupAt(window->weak_map_id());
  } else {
    // Passing -1 (invalid) would make closePopupAt close all menu runners
    // belonging to this menu.
    ClosePopupAt(-1);
  }
}

// static
void Menu::SetApplicationMenuFromJS(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);
  v8::Local<v8::Value> value;
  args->GetNext(&value);
  Menu* menu = nullptr;
  if (!value.IsEmpty() && value->BooleanValue(isolate)) {
    if (!value->IsObject() || !gin::ConvertFromV8(isolate, value, &menu) ||
        !menu) {
      thrower.ThrowTypeError("Invalid menu");
      return;
    }
  }
  g_application_menu_was_set = true;
  if (menu)
    ApplicationMenu() = cppgc::Persistent<Menu>(menu);
  else
    ApplicationMenu().Clear();
#if BUILDFLAG(IS_MAC)
  if (!menu)
    return;
  SetApplicationMenu(menu);
#else
  // Setting a menu can run app JS (e.g. 'resize'), which can open or close
  // windows; iterate a copy.
  std::vector<BaseWindow*> windows = BaseWindow::GetAllNative();
  for (BaseWindow* window : windows) {
    if (!BaseWindow::IsLive(window))
      continue;
    if (menu)
      window->SetMenuNatively(menu);
    else
      window->RemoveMenu();
  }
#endif
}

// static
v8::Local<v8::Value> Menu::GetApplicationMenu(v8::Isolate* isolate) {
  Menu* menu = ApplicationMenu().Get();
  v8::Local<v8::Object> wrapper;
  if (menu && menu->GetWrapper(isolate).ToLocal(&wrapper))
    return wrapper;
  return v8::Null(isolate);
}

// static
bool Menu::ApplicationMenuWasSet() {
  return g_application_menu_was_set;
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
      .SetMethod("getMenuItemById", &Menu::GetMenuItemById)
      .SetMethod("popup", &Menu::Popup)
      .SetMethod("closePopup", &Menu::ClosePopup)
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
  statics.SetMethod("setApplicationMenu", &Menu::SetApplicationMenuFromJS);
  statics.SetMethod("getApplicationMenu", &Menu::GetApplicationMenu);
  statics.SetMethod("_applicationMenuWasSet", &Menu::ApplicationMenuWasSet);
  statics.SetMethod("_roleDefaults", &electron::api::menu_roles::Defaults);
#if BUILDFLAG(IS_MAC)
  statics.SetMethod("sendActionToFirstResponder",
                    &Menu::SendActionToFirstResponder);
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_menu, Initialize)

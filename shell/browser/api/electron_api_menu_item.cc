// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_menu_item.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/api/electron_api_menu_roles.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/ui_event.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/accelerator_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8.h"

namespace electron::api {

namespace {

int g_next_command_id = 0;

constexpr std::array<std::pair<std::string_view, MenuItem::Type>, 7> kTypes{{
    {"normal", MenuItem::Type::kNormal},
    {"separator", MenuItem::Type::kSeparator},
    {"submenu", MenuItem::Type::kSubmenu},
    {"checkbox", MenuItem::Type::kCheckbox},
    {"radio", MenuItem::Type::kRadio},
    {"header", MenuItem::Type::kHeader},
    {"palette", MenuItem::Type::kPalette},
}};

std::string_view TypeName(MenuItem::Type type) {
  for (const auto& [name, value] : kTypes) {
    if (value == type)
      return name;
  }
  return "normal";
}

std::u16string ToText(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  std::u16string text;
  v8::Local<v8::String> string;
  if (!value->IsNullOrUndefined() &&
      value->ToString(isolate->GetCurrentContext()).ToLocal(&string)) {
    gin::ConvertFromV8(isolate, string, &text);
  }
  return text;
}

#if BUILDFLAG(IS_MAC)
bool ValidateBadge(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  gin_helper::ErrorThrower thrower(isolate);
  if (value->IsNullOrUndefined())
    return true;
  gin_helper::Dictionary badge;
  if (!gin::ConvertFromV8(isolate, value, &badge)) {
    thrower.ThrowTypeError("badge must be a MenuItemBadge object");
    return false;
  }
  std::string type = "none";
  badge.Get("type", &type);
  static constexpr std::array<std::string_view, 4> kBadgeTypes{
      "alerts", "updates", "new-items", "none"};
  if (std::ranges::find(kBadgeTypes, type) == kBadgeTypes.end()) {
    thrower.ThrowTypeError(
        base::StrCat({"Invalid badge type '", type,
                      "': must be one of alerts, updates, new-items, none"}));
    return false;
  }
  v8::Local<v8::Value> content;
  v8::Local<v8::Value> count;
  const bool has_content =
      badge.Get("content", &content) && !content->IsNullOrUndefined();
  const bool has_count =
      badge.Get("count", &count) && !count->IsNullOrUndefined();
  if (type == "none") {
    if (!has_content || !content->IsString()) {
      thrower.ThrowTypeError(
          "badge.content must be a string when badge.type is 'none'");
      return false;
    }
    if (has_count) {
      thrower.ThrowTypeError(
          "badge.count cannot be used when badge.type is 'none'");
      return false;
    }
  } else {
    if (!has_count || !count->IsInt32() || count.As<v8::Int32>()->Value() < 0) {
      thrower.ThrowTypeError(base::StrCat(
          {"badge.count must be a non-negative integer when badge.type is '",
           type, "'"}));
      return false;
    }
    if (has_content) {
      thrower.ThrowTypeError(
          "badge.content can only be used when badge.type is 'none'");
      return false;
    }
  }
  return true;
}
#endif

bool SetIcon(v8::Isolate* isolate,
             gin_helper::ConvertedValue<gfx::Image>& icon,
             v8::Local<v8::Value> value) {
  // A falsy icon means none.
  if (!value->BooleanValue(isolate)) {
    icon.Reset();
    return true;
  }
  if (icon.Set(isolate, value))
    return true;
  gin_helper::ErrorThrower(isolate).ThrowTypeError(
      "icon must be a NativeImage or a path to an image");
  return false;
}

}  // namespace

const gin::WrapperInfo MenuItem::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronMenuItem);

MenuItem::MenuItem() : command_id_(++g_next_command_id) {}
MenuItem::~MenuItem() = default;

// static
MenuItem* MenuItem::FromV8(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  MenuItem* item = nullptr;
  if (value.IsEmpty() || !value->IsObject() ||
      !gin::ConvertFromV8(isolate, value, &item)) {
    return nullptr;
  }
  return item;
}

// static
MenuItem* MenuItem::Create(v8::Isolate* isolate) {
  MenuItem* item = cppgc::MakeGarbageCollected<MenuItem>(
      isolate->GetCppHeap()->GetAllocationHandle());
  v8::Local<v8::Object> wrapper;
  if (!item->GetWrapper(isolate).ToLocal(&wrapper))
    return nullptr;
  return item;
}

// static
MenuItem* MenuItem::NewWithRole(v8::Isolate* isolate,
                                const menu_roles::Role& role) {
  MenuItem* item = Create(isolate);
  if (!item)
    return nullptr;
  item->role_name_ = role.id;
  item->role_ = &role;
  item->has_role_ = true;
  item->ApplyRoleDefaults(isolate, false, false, false, false);
  return item;
}

// static
MenuItem* MenuItem::NewSeparator(v8::Isolate* isolate) {
  MenuItem* item = Create(isolate);
  if (item)
    item->type_ = Type::kSeparator;
  return item;
}

// static
MenuItem* MenuItem::NewSubmenu(v8::Isolate* isolate,
                               std::u16string label,
                               Menu* submenu) {
  MenuItem* item = Create(isolate);
  if (!item)
    return nullptr;
  item->type_ = Type::kSubmenu;
  item->label_ = std::move(label);
  item->submenu_ = submenu;
  return item;
}

// static
v8::Local<v8::Value> MenuItem::New(gin_helper::ErrorThrower thrower,
                                   v8::Local<v8::Value> options) {
  v8::Isolate* isolate = thrower.isolate();
  if (options.IsEmpty() || !options->IsObject()) {
    thrower.ThrowTypeError("MenuItem options must be an object");
    return {};
  }
  MenuItem* item = Create(isolate);
  v8::Local<v8::Object> wrapper;
  if (!item || !item->GetWrapper(isolate).ToLocal(&wrapper) ||
      !item->Init(thrower, wrapper, options.As<v8::Object>())) {
    return {};
  }
  return wrapper;
}

bool MenuItem::Init(gin_helper::ErrorThrower thrower,
                    v8::Local<v8::Object> wrapper,
                    v8::Local<v8::Object> options_object) {
  v8::Isolate* isolate = thrower.isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  gin_helper::Dictionary options(isolate, options_object);

  // Preserve extra fields specified by the app, without shadowing the
  // item's own properties.
  v8::Local<v8::Array> keys;
  if (options_object->GetPropertyNames(context).ToLocal(&keys)) {
    for (uint32_t i = 0, n = keys->Length(); i < n; ++i) {
      v8::Local<v8::Value> key, value;
      if (!keys->Get(context, i).ToLocal(&key) || !key->IsName() ||
          wrapper->Has(context, key.As<v8::Name>()).FromMaybe(true)) {
        continue;
      }
      if (options_object->Get(context, key).ToLocal(&value))
        wrapper->Set(context, key, value).Check();
    }
  }

  if (options.Get("role", &role_name_)) {
    role_name_ = base::ToLowerASCII(role_name_);
    role_ = menu_roles::Find(role_name_);
    has_role_ = true;
  }

  v8::Local<v8::Value> submenu_value;
  if (options.Get("submenu", &submenu_value) &&
      submenu_value->BooleanValue(isolate)) {
    Menu* submenu = nullptr;
    if (!submenu_value->IsObject() ||
        !gin::ConvertFromV8(isolate, submenu_value, &submenu) || !submenu) {
      v8::Local<v8::Value> built =
          Menu::BuildFromTemplate(thrower, submenu_value);
      if (built.IsEmpty() || !gin::ConvertFromV8(isolate, built, &submenu))
        return false;
    }
    submenu_ = submenu;
  }

  std::string type;
  const bool has_type = options.Get("type", &type);
  if (has_type) {
    const auto found = std::ranges::find(
        kTypes, type, [](const auto& entry) { return entry.first; });
    if (found == kTypes.end()) {
      thrower.ThrowError("Unknown menu item type: " + type);
      return false;
    }
    type_ = found->second;
  }
  // Any non-null accelerator counts as given; one that is not a string means
  // no accelerator rather than the role's default.
  v8::Local<v8::Value> accelerator;
  has_accelerator_ = options.Get("accelerator", &accelerator) &&
                     !accelerator->IsNullOrUndefined();
  if (has_accelerator_)
    gin::ConvertFromV8(isolate, accelerator, &accelerator_string_);
  const bool has_label = options.Get("label", &label_);
  // gin's bool conversion accepts null/undefined; those mean the default.
  auto get_flag = [&](std::string_view key, bool* out) {
    v8::Local<v8::Value> value;
    if (!options.Get(key, &value) || value->IsNullOrUndefined())
      return false;
    *out = value->BooleanValue(isolate);
    return true;
  };
  const bool has_register =
      get_flag("registerAccelerator", &register_accelerator_);
  ApplyRoleDefaults(isolate, has_type, has_accelerator_, has_label,
                    has_register);
  if (type_ == Type::kSubmenu && !submenu_) {
    thrower.ThrowError("Invalid submenu");
    return false;
  }

  options.Get("sublabel", &sublabel_);
  options.Get("toolTip", &tool_tip_);
  options.Get("accessibilityLabel", &accessibility_label_);
  get_flag("enabled", &enabled_);
  get_flag("visible", &visible_);
  get_flag("checked", &checked_);
  get_flag("acceleratorWorksWhenHidden", &works_when_hidden_);

  v8::Local<v8::Value> value;
  if (options.Get("icon", &value) && !SetIcon(isolate, icon_, value))
    return false;
  if (options.Get("click", &value) && value->IsFunction())
    user_click_.Reset(isolate, value);
#if BUILDFLAG(IS_MAC)
  if (options.Get("sharingItem", &value))
    sharing_item_.Set(isolate, value);
  if (options.Get("badge", &value)) {
    if (!ValidateBadge(isolate, value))
      return false;
    badge_.Set(isolate, value);
  }
  options.Get("selector", &selector_);
#endif
  return true;
}

void MenuItem::ApplyRoleDefaults(v8::Isolate* isolate,
                                 bool has_type,
                                 bool has_accelerator,
                                 bool has_label,
                                 bool has_register_accelerator) {
  if (!submenu_ && role_)
    submenu_ = menu_roles::DefaultSubmenu(isolate, *role_);
  if (!has_type) {
    type_ = submenu_                               ? Type::kSubmenu
            : (role_ && role_->computes_checked()) ? Type::kCheckbox
                                                   : Type::kNormal;
  }
  if (!has_accelerator && role_ && *role_->accelerator) {
    accelerator_string_ = role_->accelerator;
    has_accelerator_ = true;
  }
  if (has_accelerator_) {
    ui::Accelerator accelerator;
    if (gin::ConvertFromV8(isolate,
                           gin::StringToV8(isolate, accelerator_string_),
                           &accelerator)) {
      accelerator_ = accelerator;
    }
  }
  if (!has_label && role_)
    label_ = role_->Label();
  if (!has_register_accelerator && role_)
    register_accelerator_ = role_->register_accelerator;
}

void MenuItem::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<MenuItem>::Trace(visitor);
  visitor->Trace(submenu_);
  visitor->Trace(menu_);
  visitor->Trace(radio_menu_);
  visitor->Trace(icon_);
#if BUILDFLAG(IS_MAC)
  visitor->Trace(sharing_item_);
  visitor->Trace(badge_);
#endif
  visitor->Trace(user_click_);
  visitor->Trace(click_);
  visitor->Trace(replaced_click_);
}

const gin::WrapperInfo* MenuItem::wrapper_info() const {
  return &kWrapperInfo;
}

const char* MenuItem::GetHumanReadableName() const {
  return "Electron / MenuItem";
}

ui::MenuModel::ItemType MenuItem::GetType() const {
  switch (type_) {
    case Type::kNormal:
    case Type::kHeader:
      return ui::MenuModel::TYPE_COMMAND;
    case Type::kSeparator:
      return ui::MenuModel::TYPE_SEPARATOR;
    case Type::kCheckbox:
      return ui::MenuModel::TYPE_CHECK;
    case Type::kRadio:
      return ui::MenuModel::TYPE_RADIO;
    case Type::kSubmenu:
    case Type::kPalette:
      return ui::MenuModel::TYPE_SUBMENU;
  }
}

int MenuItem::GetCommandId() const {
  return command_id_;
}

std::u16string MenuItem::GetLabel() const {
  return label_;
}

std::u16string MenuItem::GetSecondaryLabel() const {
  return sublabel_;
}

std::u16string MenuItem::GetToolTip() const {
  return tool_tip_;
}

std::u16string MenuItem::GetAccessibilityLabel() const {
  return accessibility_label_;
}

std::u16string MenuItem::GetRole() const {
  return base::UTF8ToUTF16(role_name_);
}

std::u16string MenuItem::GetCustomType() const {
  if (type_ == Type::kPalette)
    return u"palette";
  if (type_ == Type::kHeader)
    return u"header";
  return {};
}

ui::ImageModel MenuItem::GetIcon() const {
  const gfx::Image* image = icon_.Get();
  return image ? ui::ImageModel::FromImage(*image) : ui::ImageModel();
}

bool MenuItem::GetAccelerator(ui::Accelerator* accelerator) const {
  if (!accelerator_)
    return false;
  *accelerator = *accelerator_;
  return true;
}

bool MenuItem::ShouldRegisterAccelerator() const {
  return register_accelerator_;
}

bool MenuItem::WorksWhenHidden() const {
  return works_when_hidden_;
}

bool MenuItem::IsVisible() const {
  return visible_;
}

int MenuItem::GetGroupId() const {
  return type_ == Type::kRadio ? group_id_ : -1;
}

ElectronMenuModel* MenuItem::GetSubmenuModel() const {
  if ((type_ == Type::kSubmenu || type_ == Type::kPalette) && submenu_)
    return submenu_->model();
  return nullptr;
}

bool MenuItem::IsChecked() const {
  if (role_ && role_->computes_checked())
    return menu_roles::IsChecked(*role_);
  return checked_;
}

bool MenuItem::IsEnabled() const {
  if (role_) {
    if (std::optional<bool> enabled = menu_roles::IsEnabled(*role_))
      return *enabled;
  }
  return enabled_;
}

#if BUILDFLAG(IS_MAC)
std::optional<ElectronMenuModel::SharingItem> MenuItem::GetSharingItem() {
  const ElectronMenuModel::SharingItem* item =
      sharing_item_.Refresh(JavascriptEnvironment::GetIsolate());
  return item ? std::make_optional(*item) : std::nullopt;
}

const ElectronMenuModel::Badge* MenuItem::GetBadge() const {
  return badge_.Get();
}
#endif

void MenuItem::SetChecked(bool checked) {
  if (type_ == Type::kRadio && radio_menu_) {
    SetCheckedForRadioGroup();
    return;
  }
  checked_ = checked;
}

void MenuItem::SetCheckedForRadioGroup() {
  if (radio_menu_) {
    radio_menu_->ForEachInRadioGroup(group_id_, [this](MenuItem* other) {
      if (other != this)
        other->checked_ = false;
    });
  }
  checked_ = true;
}

void MenuItem::AttachToMenu(Menu* menu, int group_id) {
  if (!menu_)
    menu_ = menu;
  if (type_ == Type::kRadio) {
    radio_menu_ = menu;
    if (!group_id_)
      group_id_ = group_id;
  }
}

void MenuItem::Activate(BaseWindow* window,
                        WebContents* web_contents,
                        int flags) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> window_value = window && !window->GetWrapper().IsEmpty()
                                          ? window->GetWrapper().As<v8::Value>()
                                          : v8::Null(isolate).As<v8::Value>();
  if (replaced_click_.IsEmpty()) {
    if (!RunBuiltInAction(window, web_contents))
      CallClick(isolate, window_value, CreateEventFromFlags(flags));
    return;
  }
  // click(event, focusedWindow, focusedWebContents)
  v8::Local<v8::Value> click = replaced_click_.Get(isolate);
  v8::Local<v8::Object> wrapper;
  if (!click->IsFunction() || !GetWrapper(isolate).ToLocal(&wrapper))
    return;
  v8::Local<v8::Context> context = wrapper->GetCreationContextChecked(isolate);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> contents_wrapper;
  v8::Local<v8::Value> contents_value = v8::Null(isolate);
  if (web_contents &&
      web_contents->GetWrapper(isolate).ToLocal(&contents_wrapper)) {
    contents_value = contents_wrapper;
  }
  v8::Local<v8::Value> argv[] = {CreateEventFromFlags(flags), window_value,
                                 contents_value};
  std::ignore =
      click.As<v8::Function>()->Call(context, wrapper, std::size(argv), argv);
}

bool MenuItem::RunBuiltInAction(BaseWindow* window, WebContents* web_contents) {
  const bool computes_checked = role_ && role_->computes_checked();
  if (!computes_checked &&
      (type_ == Type::kCheckbox || type_ == Type::kRadio)) {
    SetChecked(!checked_);
  }
  if (role_ && menu_roles::Execute(*role_, window, web_contents))
    return true;
  if (!user_click_.IsEmpty())
    return false;
#if BUILDFLAG(IS_MAC)
  if (!selector_.empty())
    Menu::SendActionToFirstResponder(selector_);
#endif
  return true;
}

void MenuItem::CallClick(v8::Isolate* isolate,
                         v8::Local<v8::Value> window,
                         v8::Local<v8::Value> event) {
  v8::Local<v8::Object> wrapper;
  v8::Local<v8::Value> click = user_click_.Get(isolate);
  if (!click->IsFunction() || !GetWrapper(isolate).ToLocal(&wrapper))
    return;
  v8::Local<v8::Context> context = wrapper->GetCreationContextChecked(isolate);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Value> argv[] = {wrapper, window, event};
  std::ignore =
      click.As<v8::Function>()->Call(context, wrapper, std::size(argv), argv);
}

// static
void MenuItem::ClickThunk(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  MenuItem* item = FromV8(isolate, info.Data());
  if (!item)
    return;
  BaseWindow* window = BaseWindow::FromValue(isolate, info[1]);
  WebContents* web_contents = nullptr;
  if (gin_helper::IsValidWrappable(info[2], &WebContents::kWrapperInfo))
    gin::ConvertFromV8(isolate, info[2], &web_contents);
  if (!item->RunBuiltInAction(window, web_contents))
    item->CallClick(isolate, info[1], info[0]);
}

// static
void MenuItem::UserAcceleratorThunk(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().SetNull();
#if BUILDFLAG(IS_MAC)
  MenuItem* self = FromV8(info.GetIsolate(), info.This());
  if (self && self->menu_) {
    info.GetReturnValue().Set(
        self->menu_->GetUserAcceleratorAt(self->command_id_));
  }
#endif
}

v8::Local<v8::Value> MenuItem::GetDefaultRoleAccelerator(
    v8::Isolate* isolate) const {
  if (!role_ || !*role_->accelerator)
    return v8::Undefined(isolate);
  return gin::StringToV8(isolate, role_->accelerator);
}

template <MenuItem::Getter getter>
// static
void MenuItem::GetterThunk(v8::Local<v8::Name> name,
                           const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  MenuItem* self = FromV8(isolate, info.Holder());
  if (self)
    info.GetReturnValue().Set(getter(self, isolate));
}

template <MenuItem::Setter setter>
// static
void MenuItem::SetterThunk(v8::Local<v8::Name> name,
                           v8::Local<v8::Value> value,
                           const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  MenuItem* self = FromV8(isolate, info.Holder());
  if (self)
    setter(self, isolate, value);
}

template <MenuItem::Getter getter, MenuItem::Setter setter>
// static
void MenuItem::DefineProperty(v8::Isolate* isolate,
                              v8::Local<v8::ObjectTemplate> templ,
                              std::string_view name) {
  if constexpr (setter == nullptr) {
    templ->SetNativeDataProperty(
        gin::StringToSymbol(isolate, name), &GetterThunk<getter>, nullptr,
        v8::Local<v8::Value>(),
        static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete),
        v8::SideEffectType::kHasNoSideEffect);
  } else {
    templ->SetNativeDataProperty(gin::StringToSymbol(isolate, name),
                                 &GetterThunk<getter>, &SetterThunk<setter>,
                                 v8::Local<v8::Value>(), v8::DontDelete,
                                 v8::SideEffectType::kHasNoSideEffect);
  }
}

// static
void MenuItem::FillObjectTemplate(v8::Isolate* isolate,
                                  v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, "MenuItem", templ)
      .SetMethod("getDefaultRoleAccelerator",
                 &MenuItem::GetDefaultRoleAccelerator)
      .Build();
}

#define TEXT_PROPERTY(field)                                             \
  [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {     \
    return gin::ConvertToV8(isolate, self->field);                       \
  },                                                                     \
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) { \
        self->field = ToText(isolate, v);                                \
      }
#define FLAG_PROPERTY(field)                                             \
  [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {     \
    return v8::Boolean::New(isolate, self->field);                       \
  },                                                                     \
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) { \
        self->field = v->BooleanValue(isolate);                          \
      }

// static
void MenuItem::FillInstanceTemplate(v8::Isolate* isolate,
                                    v8::Local<v8::ObjectTemplate> templ) {
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    return v8::Integer::New(isolate, self->command_id_);
  }>(isolate, templ, "commandId");
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    return gin::StringToSymbol(isolate, TypeName(self->type_));
  }>(isolate, templ, "type");
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    if (!self->has_role_)
      return v8::Null(isolate);
    return gin::StringToV8(isolate, self->role_name_);
  }>(isolate, templ, "role");
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    if (!self->has_accelerator_)
      return v8::Null(isolate);
    return gin::StringToV8(isolate, self->accelerator_string_);
  }>(isolate, templ, "accelerator");
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    v8::Local<v8::Object> wrapper;
    if (self->submenu_ && self->submenu_->GetWrapper(isolate).ToLocal(&wrapper))
      return wrapper;
    return v8::Null(isolate);
  }>(isolate, templ, "submenu");
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    v8::Local<v8::Object> wrapper;
    if (self->menu_ && self->menu_->GetWrapper(isolate).ToLocal(&wrapper))
      return wrapper;
    return v8::Undefined(isolate);
  }>(isolate, templ, "menu");
  DefineProperty<[](MenuItem* self,
                    v8::Isolate* isolate) -> v8::Local<v8::Value> {
    if (self->type_ != Type::kRadio || !self->menu_)
      return v8::Undefined(isolate);
    return v8::Integer::New(isolate, self->group_id_);
  }>(isolate, templ, "groupId");
  // An accessor rather than a data property so that inspecting an item does
  // not evaluate it (on macOS it builds an NSMenu).
  templ->SetAccessorProperty(
      gin::StringToSymbol(isolate, "userAccelerator"),
      v8::FunctionTemplate::New(isolate, UserAcceleratorThunk),
      v8::Local<v8::FunctionTemplate>(),
      static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));

  DefineProperty<TEXT_PROPERTY(label_)>(isolate, templ, "label");
  DefineProperty<TEXT_PROPERTY(sublabel_)>(isolate, templ, "sublabel");
  DefineProperty<TEXT_PROPERTY(tool_tip_)>(isolate, templ, "toolTip");
  DefineProperty<TEXT_PROPERTY(accessibility_label_)>(isolate, templ,
                                                      "accessibilityLabel");
  DefineProperty<FLAG_PROPERTY(enabled_)>(isolate, templ, "enabled");
  DefineProperty<FLAG_PROPERTY(visible_)>(isolate, templ, "visible");
  DefineProperty<FLAG_PROPERTY(works_when_hidden_)>(
      isolate, templ, "acceleratorWorksWhenHidden");
  DefineProperty<FLAG_PROPERTY(register_accelerator_)>(isolate, templ,
                                                       "registerAccelerator");
  DefineProperty<
      [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {
        return v8::Boolean::New(isolate, self->checked_);
      },
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) {
        self->SetChecked(v->BooleanValue(isolate));
      }>(isolate, templ, "checked");
  DefineProperty<
      [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {
        return self->icon_.GetV8(isolate, v8::Null(isolate));
      },
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) {
        SetIcon(isolate, self->icon_, v);
      }>(isolate, templ, "icon");
  DefineProperty<
      [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {
        if (!self->replaced_click_.IsEmpty())
          return self->replaced_click_.Get(isolate);
        if (self->click_.IsEmpty()) {
          v8::Local<v8::Object> wrapper;
          v8::Local<v8::Function> click;
          if (!self->GetWrapper(isolate).ToLocal(&wrapper) ||
              !v8::Function::New(isolate->GetCurrentContext(), ClickThunk,
                                 wrapper, 3, v8::ConstructorBehavior::kThrow)
                   .ToLocal(&click)) {
            return v8::Undefined(isolate);
          }
          self->click_.Reset(isolate, click);
        }
        return self->click_.Get(isolate);
      },
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) {
        self->replaced_click_.Reset(isolate, v);
      }>(isolate, templ, "click");
#if BUILDFLAG(IS_MAC)
  DefineProperty<
      [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {
        return self->sharing_item_.GetV8(isolate, v8::Undefined(isolate));
      },
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) {
        self->sharing_item_.Set(isolate, v);
      }>(isolate, templ, "sharingItem");
  DefineProperty<
      [](MenuItem* self, v8::Isolate* isolate) -> v8::Local<v8::Value> {
        return self->badge_.GetV8(isolate, v8::Undefined(isolate));
      },
      [](MenuItem* self, v8::Isolate* isolate, v8::Local<v8::Value> v) {
        if (!ValidateBadge(isolate, v))
          return;
        self->badge_.Set(isolate, v);
      }>(isolate, templ, "badge");
#endif
}

#undef TEXT_PROPERTY
#undef FLAG_PROPERTY

}  // namespace electron::api

// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_ITEM_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_ITEM_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "gin/wrappable.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/converted_value.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/image/image.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/v8-traced-handle.h"

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace electron::api {

class BaseWindow;
class Menu;
class WebContents;

namespace menu_roles {
struct Role;
}

class MenuItem final : public gin::Wrappable<MenuItem>,
                       public gin_helper::Constructible<MenuItem> {
 public:
  enum class Type {
    kNormal,
    kSeparator,
    kSubmenu,
    kCheckbox,
    kRadio,
    kHeader,
    kPalette
  };

  static v8::Local<v8::Value> New(gin_helper::ErrorThrower thrower,
                                  v8::Local<v8::Value> options);
  // For the role submenus.
  static MenuItem* NewWithRole(v8::Isolate* isolate,
                               const menu_roles::Role& role);
  static MenuItem* NewSeparator(v8::Isolate* isolate);
  static MenuItem* NewSubmenu(v8::Isolate* isolate,
                              std::u16string label,
                              Menu* submenu);
  static MenuItem* FromV8(v8::Isolate* isolate, v8::Local<v8::Value> value);
  static MenuItem* FromCommandId(int command_id);

  MenuItem();
  ~MenuItem() override;

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor*) const override;

  // gin_helper::Constructible
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static void FillInstanceTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "MenuItem"; }

  int command_id() const { return command_id_; }
  Type type() const { return type_; }
  const std::string& role_name() const { return role_name_; }
  const std::u16string& label() const { return label_; }
  const std::u16string& sublabel() const { return sublabel_; }
  const std::u16string& tool_tip() const { return tool_tip_; }
  const std::u16string& accessibility_label() const {
    return accessibility_label_;
  }
  bool visible() const { return visible_; }
  bool works_when_hidden() const { return works_when_hidden_; }
  bool register_accelerator() const { return register_accelerator_; }
  const std::optional<ui::Accelerator>& accelerator() const {
    return accelerator_;
  }
  ui::ImageModel icon() const;
  int group_id() const { return group_id_; }
  Menu* submenu() const { return submenu_.Get(); }
  Menu* menu() const { return menu_.Get(); }
  bool checked_flag() const { return checked_; }
  bool IsChecked() const;
  bool IsEnabled() const;
#if BUILDFLAG(IS_MAC)
  // Re-read on each call: apps update the object in place.
  const ElectronMenuModel::SharingItem* GetSharingItem(v8::Isolate* isolate);
  const ElectronMenuModel::Badge* badge() const { return badge_.Get(); }
#endif

  void AttachToMenu(Menu* menu, int group_id);
  // What choosing the item in a menu does: the app's replacement for
  // `item.click` if it assigned one, else the default action.
  void Activate(BaseWindow* window, WebContents* web_contents, int flags);
  void SetCheckedForRadioGroup();

 private:
  bool Init(gin_helper::ErrorThrower thrower,
            v8::Local<v8::Object> wrapper,
            v8::Local<v8::Object> options);
  void ApplyRoleDefaults(v8::Isolate* isolate,
                         bool has_type,
                         bool has_accelerator,
                         bool has_label,
                         bool has_register_accelerator);
  static MenuItem* Create(v8::Isolate* isolate);
  void SetChecked(bool checked);
  // Flips checkbox/radio state and runs the role; false if there is no role
  // action and the app's `click` should run.
  bool RunBuiltInAction(BaseWindow* window, WebContents* web_contents);
  // click(menuItem, window, event)
  void CallClick(v8::Isolate* isolate,
                 v8::Local<v8::Value> window,
                 v8::Local<v8::Value> event);
  v8::Local<v8::Value> GetDefaultRoleAccelerator(v8::Isolate* isolate) const;

  using Getter = v8::Local<v8::Value> (*)(MenuItem* self, v8::Isolate* isolate);
  using Setter = void (*)(MenuItem* self,
                          v8::Isolate* isolate,
                          v8::Local<v8::Value> value);
  template <Getter getter>
  static void GetterThunk(v8::Local<v8::Name> name,
                          const v8::PropertyCallbackInfo<v8::Value>& info);
  template <Setter setter>
  static void SetterThunk(v8::Local<v8::Name> name,
                          v8::Local<v8::Value> value,
                          const v8::PropertyCallbackInfo<v8::Boolean>& info);
  template <Getter getter, Setter setter = nullptr>
  static void DefineProperty(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> templ,
                             std::string_view name);
  // item.click(event, focusedWindow, focusedWebContents)
  static void ClickThunk(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void UserAcceleratorThunk(
      const v8::FunctionCallbackInfo<v8::Value>& info);

  int command_id_ = 0;
  int group_id_ = 0;
  Type type_ = Type::kNormal;
  bool has_role_ = false;
  std::string role_name_;
  raw_ptr<const menu_roles::Role> role_ = nullptr;
  std::u16string label_;
  std::u16string sublabel_;
  std::u16string tool_tip_;
  std::u16string accessibility_label_;
  bool enabled_ = true;
  bool visible_ = true;
  bool checked_ = false;
  bool works_when_hidden_ = true;
  bool register_accelerator_ = true;
  bool has_accelerator_ = false;
  std::string accelerator_string_;
  std::optional<ui::Accelerator> accelerator_;
  std::string selector_;

  cppgc::Member<Menu> submenu_;
  cppgc::Member<Menu> menu_;
  // The menu whose radio group |group_id_| refers to (the latest insert).
  cppgc::Member<Menu> radio_menu_;
  gin_helper::ConvertedValue<gfx::Image> icon_;
#if BUILDFLAG(IS_MAC)
  gin_helper::ConvertedValue<ElectronMenuModel::SharingItem> sharing_item_;
  gin_helper::ConvertedValue<ElectronMenuModel::Badge> badge_;
#endif
  // `click` from the options, the bound `item.click`, and a function the app
  // assigned to `item.click` afterwards (which replaces the default action).
  v8::TracedReference<v8::Value> user_click_;
  v8::TracedReference<v8::Function> click_;
  v8::TracedReference<v8::Value> replaced_click_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_ITEM_H_

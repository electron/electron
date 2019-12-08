// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_MENU_H_
#define SHELL_BROWSER_API_ATOM_API_MENU_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "gin/arguments.h"
#include "shell/browser/api/atom_api_top_level_window.h"
#include "shell/browser/ui/atom_menu_model.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace electron {

namespace api {

class Menu : public gin_helper::TrackableObject<Menu>,
             public AtomMenuModel::Delegate,
             public AtomMenuModel::Observer {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if defined(OS_MACOSX)
  // Set the global menubar.
  static void SetApplicationMenu(Menu* menu);

  // Fake sending an action from the application menu.
  static void SendActionToFirstResponder(const std::string& action);
#endif

  AtomMenuModel* model() const { return model_.get(); }

 protected:
  explicit Menu(gin::Arguments* args);
  ~Menu() override;

  // gin_helper::Wrappable:
  void AfterInit(v8::Isolate* isolate) override;

  // ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool ShouldCommandIdWorkWhenHidden(int command_id) const override;
  bool GetAcceleratorForCommandIdWithParams(
      int command_id,
      bool use_default_accelerator,
      ui::Accelerator* accelerator) const override;
  bool ShouldRegisterAcceleratorForCommandId(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void OnMenuWillShow(ui::SimpleMenuModel* source) override;

  virtual void PopupAt(TopLevelWindow* window,
                       int x,
                       int y,
                       int positioning_item,
                       base::OnceClosure callback) = 0;
  virtual void ClosePopupAt(int32_t window_id) = 0;

  std::unique_ptr<AtomMenuModel> model_;
  Menu* parent_ = nullptr;

  // Observable:
  void OnMenuWillClose() override;
  void OnMenuWillShow() override;

 protected:
  // Returns a new callback which keeps references of the JS wrapper until the
  // passed |callback| is called.
  base::OnceClosure BindSelfToClosure(base::OnceClosure callback);

 private:
  void InsertItemAt(int index, int command_id, const base::string16& label);
  void InsertSeparatorAt(int index);
  void InsertCheckItemAt(int index,
                         int command_id,
                         const base::string16& label);
  void InsertRadioItemAt(int index,
                         int command_id,
                         const base::string16& label,
                         int group_id);
  void InsertSubMenuAt(int index,
                       int command_id,
                       const base::string16& label,
                       Menu* menu);
  void SetIcon(int index, const gfx::Image& image);
  void SetSublabel(int index, const base::string16& sublabel);
  void SetToolTip(int index, const base::string16& toolTip);
  void SetRole(int index, const base::string16& role);
  void Clear();
  int GetIndexOfCommandId(int command_id);
  int GetItemCount() const;
  int GetCommandIdAt(int index) const;
  base::string16 GetLabelAt(int index) const;
  base::string16 GetSublabelAt(int index) const;
  base::string16 GetToolTipAt(int index) const;
  base::string16 GetAcceleratorTextAt(int index) const;
  bool IsItemCheckedAt(int index) const;
  bool IsEnabledAt(int index) const;
  bool IsVisibleAt(int index) const;
  bool WorksWhenHiddenAt(int index) const;

  // Stored delegate methods.
  base::RepeatingCallback<bool(v8::Local<v8::Value>, int)> is_checked_;
  base::RepeatingCallback<bool(v8::Local<v8::Value>, int)> is_enabled_;
  base::RepeatingCallback<bool(v8::Local<v8::Value>, int)> is_visible_;
  base::RepeatingCallback<bool(v8::Local<v8::Value>, int)> works_when_hidden_;
  base::RepeatingCallback<v8::Local<v8::Value>(v8::Local<v8::Value>, int, bool)>
      get_accelerator_;
  base::RepeatingCallback<bool(v8::Local<v8::Value>, int)>
      should_register_accelerator_;
  base::RepeatingCallback<void(v8::Local<v8::Value>, v8::Local<v8::Value>, int)>
      execute_command_;
  base::RepeatingCallback<void(v8::Local<v8::Value>)> menu_will_show_;

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace electron

namespace gin {

template <>
struct Converter<electron::AtomMenuModel*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::AtomMenuModel** out) {
    // null would be tranfered to NULL.
    if (val->IsNull()) {
      *out = nullptr;
      return true;
    }

    electron::api::Menu* menu;
    if (!Converter<electron::api::Menu*>::FromV8(isolate, val, &menu))
      return false;
    *out = menu->model();
    return true;
  }
};

}  // namespace gin

#endif  // SHELL_BROWSER_API_ATOM_API_MENU_H_

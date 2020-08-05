// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_MENU_H_
#define SHELL_BROWSER_API_ELECTRON_API_MENU_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "gin/arguments.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/pinnable.h"

namespace electron {

namespace api {

class Menu : public gin::Wrappable<Menu>,
             public gin_helper::EventEmitterMixin<Menu>,
             public gin_helper::Constructible<Menu>,
             public gin_helper::Pinnable<Menu>,
             public ElectronMenuModel::Delegate,
             public ElectronMenuModel::Observer {
 public:
  // gin_helper::Constructible
  static gin::Handle<Menu> New(gin::Arguments* args);
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;

#if defined(OS_MACOSX)
  // Set the global menubar.
  static void SetApplicationMenu(Menu* menu);

  // Fake sending an action from the application menu.
  static void SendActionToFirstResponder(const std::string& action);
#endif

  ElectronMenuModel* model() const { return model_.get(); }

 protected:
  explicit Menu(gin::Arguments* args);
  ~Menu() override;

  // Returns a new callback which keeps references of the JS wrapper until the
  // passed |callback| is called.
  base::OnceClosure BindSelfToClosure(base::OnceClosure callback);

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

  virtual void PopupAt(BaseWindow* window,
                       int x,
                       int y,
                       int positioning_item,
                       base::OnceClosure callback) = 0;
  virtual void ClosePopupAt(int32_t window_id) = 0;

  std::unique_ptr<ElectronMenuModel> model_;
  Menu* parent_ = nullptr;

  // Observable:
  void OnMenuWillClose() override;
  void OnMenuWillShow() override;

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

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace electron

namespace gin {

template <>
struct Converter<electron::ElectronMenuModel*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ElectronMenuModel** out) {
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

#endif  // SHELL_BROWSER_API_ELECTRON_API_MENU_H_

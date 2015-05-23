// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_H_

#include <string>

#include "atom/browser/api/atom_api_window.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "ui/base/models/simple_menu_model.h"
#include "native_mate/wrappable.h"

namespace atom {

namespace api {

class Menu : public mate::Wrappable,
             public ui::SimpleMenuModel::Delegate {
 public:
  static mate::Wrappable* Create();

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

#if defined(OS_MACOSX)
  // Set the global menubar.
  static void SetApplicationMenu(Menu* menu);

  // Fake sending an action from the application menu.
  static void SendActionToFirstResponder(const std::string& action);
#endif

  ui::SimpleMenuModel* model() const { return model_.get(); }

 protected:
  Menu();
  virtual ~Menu();

  // mate::Wrappable:
  void AfterInit(v8::Isolate* isolate) override;

  // ui::SimpleMenuModel::Delegate implementations:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void MenuWillShow(ui::SimpleMenuModel* source) override;

  virtual void AttachToWindow(Window* window);
  virtual void Popup(Window* window) = 0;
  virtual void PopupAt(Window* window, int x, int y) = 0;

  scoped_ptr<ui::SimpleMenuModel> model_;
  Menu* parent_;

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
  void Clear();
  int GetIndexOfCommandId(int command_id);
  int GetItemCount() const;
  int GetCommandIdAt(int index) const;
  base::string16 GetLabelAt(int index) const;
  base::string16 GetSublabelAt(int index) const;
  bool IsItemCheckedAt(int index) const;
  bool IsEnabledAt(int index) const;
  bool IsVisibleAt(int index) const;

  // Stored delegate methods.
  base::Callback<bool(int)> is_checked_;
  base::Callback<bool(int)> is_enabled_;
  base::Callback<bool(int)> is_visible_;
  base::Callback<v8::Local<v8::Value>(int)> get_accelerator_;
  base::Callback<void(int)> execute_command_;
  base::Callback<void()> menu_will_show_;

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_H_

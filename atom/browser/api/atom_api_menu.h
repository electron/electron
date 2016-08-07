// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/ui/atom_menu_model.h"
#include "base/callback.h"

namespace atom {

namespace api {

class Menu : public mate::TrackableObject<Menu>,
             public AtomMenuModel::Delegate {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

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
  Menu(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);
  ~Menu() override;

  // mate::Wrappable:
  void AfterInit(v8::Isolate* isolate) override;

  // ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool GetAcceleratorForCommandIdWithParams(
      int command_id,
      bool use_default_accelerator,
      ui::Accelerator* accelerator) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
  void MenuWillShow(ui::SimpleMenuModel* source) override;

  virtual void PopupAt(Window* window,
                       int x = -1, int y = -1,
                       int positioning_item = 0) = 0;

  std::unique_ptr<AtomMenuModel> model_;
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
  void SetRole(int index, const base::string16& role);
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
  base::Callback<v8::Local<v8::Value>(int, bool)> get_accelerator_;
  base::Callback<void(v8::Local<v8::Value>, int)> execute_command_;
  base::Callback<void()> menu_will_show_;

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace atom


namespace mate {

template<>
struct Converter<atom::AtomMenuModel*> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     atom::AtomMenuModel** out) {
    // null would be tranfered to NULL.
    if (val->IsNull()) {
      *out = nullptr;
      return true;
    }

    atom::api::Menu* menu;
    if (!Converter<atom::api::Menu*>::FromV8(isolate, val, &menu))
      return false;
    *out = menu->model();
    return true;
  }
};

}  // namespace mate

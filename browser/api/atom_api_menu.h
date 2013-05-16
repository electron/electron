// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_H_

#include "base/memory/scoped_ptr.h"
#include "browser/api/atom_api_event_emitter.h"
#include "ui/base/models/simple_menu_model.h"

namespace atom {

class NativeWindow;

namespace api {

class Menu : public EventEmitter,
             public ui::SimpleMenuModel::Delegate {
 public:
  virtual ~Menu();

  static Menu* Create(v8::Handle<v8::Object> wrapper);

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit Menu(v8::Handle<v8::Object> wrapper);

  // ui::SimpleMenuModel::Delegate implementations:
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE;
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE;
  virtual bool IsCommandIdVisible(int command_id) const OVERRIDE;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE;
  virtual bool IsItemForCommandIdDynamic(int command_id) const OVERRIDE;
  virtual string16 GetLabelForCommandId(int command_id) const OVERRIDE;
  virtual string16 GetSublabelForCommandId(int command_id) const OVERRIDE;
  virtual void ExecuteCommand(int command_id, int event_flags) OVERRIDE;

  virtual void Popup(NativeWindow* window) = 0;

  scoped_ptr<ui::SimpleMenuModel> model_;

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);

  static v8::Handle<v8::Value> InsertItem(const v8::Arguments &args);
  static v8::Handle<v8::Value> InsertCheckItem(const v8::Arguments &args);
  static v8::Handle<v8::Value> InsertRadioItem(const v8::Arguments &args);
  static v8::Handle<v8::Value> InsertSeparator(const v8::Arguments &args);
  static v8::Handle<v8::Value> InsertSubMenu(const v8::Arguments &args);

  static v8::Handle<v8::Value> SetIcon(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetSublabel(const v8::Arguments &args);

  static v8::Handle<v8::Value> Clear(const v8::Arguments &args);

  static v8::Handle<v8::Value> GetIndexOfCommandId(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetItemCount(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetCommandIdAt(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetLabelAt(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetSublabelAt(const v8::Arguments &args);
  static v8::Handle<v8::Value> IsItemCheckedAt(const v8::Arguments &args);
  static v8::Handle<v8::Value> IsEnabledAt(const v8::Arguments &args);
  static v8::Handle<v8::Value> IsVisibleAt(const v8::Arguments &args);

  static v8::Handle<v8::Value> Popup(const v8::Arguments &args);

  static v8::Handle<v8::Value> SetApplicationMenu(const v8::Arguments &args);

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_H_

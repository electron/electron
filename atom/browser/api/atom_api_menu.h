// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_H_

#include "base/memory/scoped_ptr.h"
#include "atom/common/api/atom_api_event_emitter.h"
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
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void InsertItem(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void InsertCheckItem(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void InsertRadioItem(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void InsertSeparator(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void InsertSubMenu(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void SetIcon(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetSublabel(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void Clear(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void GetIndexOfCommandId(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetItemCount(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetCommandIdAt(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetLabelAt(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetSublabelAt(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsItemCheckedAt(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsEnabledAt(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void IsVisibleAt(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void Popup(const v8::FunctionCallbackInfo<v8::Value>& args);

#if defined(OS_WIN) || defined(TOOLKIT_GTK)
  static void AttachToWindow(const v8::FunctionCallbackInfo<v8::Value>& args);
#elif defined(OS_MACOSX)
  static void SetApplicationMenu(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SendActionToFirstResponder(
      const v8::FunctionCallbackInfo<v8::Value>& args);
#endif

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_H_

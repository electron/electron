// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_H_

#include "base/memory/scoped_ptr.h"
#include "browser/api/atom_api_event_emitter.h"
#include "ui/base/models/simple_menu_model.h"

namespace atom {

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
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE;
  virtual void ExecuteCommand(int command_id, int event_flags) OVERRIDE;

  scoped_ptr<ui::SimpleMenuModel> model_;

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_H_

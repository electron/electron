// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_BUTTON_H_
#define ATOM_BROWSER_API_ATOM_API_BUTTON_H_

#include "atom/browser/api/atom_api_view.h"
#include "native_mate/handle.h"
#include "ui/views/controls/button/label_button.h"

namespace atom {

namespace api {

class Button : public View, views::ButtonListener {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 private:
  explicit Button(views::Button* view);
  ~Button() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(Button);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_BUTTON_H_

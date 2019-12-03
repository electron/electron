// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_VIEWS_ATOM_API_BUTTON_H_
#define SHELL_BROWSER_API_VIEWS_ATOM_API_BUTTON_H_

#include "gin/handle.h"
#include "shell/browser/api/atom_api_view.h"
#include "ui/views/controls/button/button.h"

namespace electron {

namespace api {

class Button : public View, public views::ButtonListener {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit Button(views::Button* view);
  ~Button() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  views::Button* button() const { return static_cast<views::Button*>(view()); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Button);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_VIEWS_ATOM_API_BUTTON_H_

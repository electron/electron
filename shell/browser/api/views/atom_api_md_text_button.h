// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_VIEWS_ATOM_API_MD_TEXT_BUTTON_H_
#define SHELL_BROWSER_API_VIEWS_ATOM_API_MD_TEXT_BUTTON_H_

#include <string>

#include "shell/browser/api/views/atom_api_label_button.h"
#include "ui/views/controls/button/md_text_button.h"

namespace electron {

namespace api {

class MdTextButton : public LabelButton {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args,
                                        const std::string& text);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit MdTextButton(const std::string& text);
  ~MdTextButton() override;

  views::MdTextButton* md_text_button() const {
    return static_cast<views::MdTextButton*>(view());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MdTextButton);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_VIEWS_ATOM_API_MD_TEXT_BUTTON_H_

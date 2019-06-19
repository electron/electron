// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_VIEWS_ATOM_API_MD_TEXT_BUTTON_H_
#define ATOM_BROWSER_API_VIEWS_ATOM_API_MD_TEXT_BUTTON_H_

#include <string>

#include "atom/browser/api/views/atom_api_label_button.h"
#include "ui/views/controls/button/md_text_button.h"

namespace atom {

namespace api {

class MdTextButton : public LabelButton {
 public:
  static mate::WrappableBase* New(mate::Arguments* args,
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

}  // namespace atom

#endif  // ATOM_BROWSER_API_VIEWS_ATOM_API_MD_TEXT_BUTTON_H_

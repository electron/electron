// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_VIEWS_ATOM_API_LABEL_BUTTON_H_
#define ATOM_BROWSER_API_VIEWS_ATOM_API_LABEL_BUTTON_H_

#include <string>

#include "atom/browser/api/views/atom_api_button.h"
#include "ui/views/controls/button/label_button.h"

namespace atom {

namespace api {

class LabelButton : public Button {
 public:
  static mate::WrappableBase* New(mate::Arguments* args,
                                  const std::string& text);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  const base::string16& GetText() const;
  void SetText(const base::string16& text);
  bool IsDefault() const;
  void SetIsDefault(bool is_default);

 protected:
  explicit LabelButton(views::LabelButton* impl);
  explicit LabelButton(const std::string& text);
  ~LabelButton() override;

  views::LabelButton* label_button() const {
    return static_cast<views::LabelButton*>(view());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LabelButton);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_VIEWS_ATOM_API_LABEL_BUTTON_H_

// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_VIEWS_ATOM_API_TEXT_FIELD_H_
#define SHELL_BROWSER_API_VIEWS_ATOM_API_TEXT_FIELD_H_

#include "gin/handle.h"
#include "shell/browser/api/atom_api_view.h"
#include "ui/views/controls/textfield/textfield.h"

namespace electron {

namespace api {

class TextField : public View {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetText(const base::string16& new_text);
  base::string16 GetText() const;

 private:
  TextField();
  ~TextField() override;

  views::Textfield* text_field() const {
    return static_cast<views::Textfield*>(view());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TextField);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_VIEWS_ATOM_API_TEXT_FIELD_H_

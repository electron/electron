// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_TEXT_FIELD_H_
#define ATOM_BROWSER_API_ATOM_API_TEXT_FIELD_H_

#include "atom/browser/api/atom_api_view.h"
#include "native_mate/handle.h"
#include "ui/views/controls/textfield/textfield.h"

namespace atom {

namespace api {

class TextField : public View {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

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

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_TEXT_FIELD_H_

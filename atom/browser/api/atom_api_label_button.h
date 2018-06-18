// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_LABEL_BUTTON_H_
#define ATOM_BROWSER_API_ATOM_API_LABEL_BUTTON_H_

#include <string>

#include "atom/browser/api/atom_api_button.h"

namespace atom {

namespace api {

class LabelButton : public Button {
 public:
  static mate::WrappableBase* New(mate::Arguments* args,
                                  const std::string& text);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit LabelButton(const std::string& text);
  ~LabelButton() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LabelButton);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_LABEL_BUTTON_H_

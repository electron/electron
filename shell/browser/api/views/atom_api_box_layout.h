// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_VIEWS_ATOM_API_BOX_LAYOUT_H_
#define SHELL_BROWSER_API_VIEWS_ATOM_API_BOX_LAYOUT_H_

#include "gin/handle.h"
#include "shell/browser/api/views/atom_api_layout_manager.h"
#include "ui/views/layout/box_layout.h"

namespace electron {

namespace api {

class View;

class BoxLayout : public LayoutManager {
 public:
  static gin_helper::WrappableBase* New(
      gin_helper::Arguments* args,
      views::BoxLayout::Orientation orientation);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetFlexForView(gin::Handle<View> view, int flex);

 protected:
  explicit BoxLayout(views::BoxLayout::Orientation orientation);
  ~BoxLayout() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BoxLayout);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_VIEWS_ATOM_API_BOX_LAYOUT_H_

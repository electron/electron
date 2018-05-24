// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_BOX_LAYOUT_H_
#define ATOM_BROWSER_API_ATOM_API_BOX_LAYOUT_H_

#include "atom/browser/api/atom_api_layout_manager.h"
#include "native_mate/handle.h"
#include "ui/views/layout/box_layout.h"

namespace atom {

namespace api {

class View;

class BoxLayout : public LayoutManager {
 public:
  static mate::WrappableBase* New(mate::Arguments* args,
                                  views::BoxLayout::Orientation orientation);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetFlexForView(mate::Handle<View> view, int flex);

 protected:
  explicit BoxLayout(views::BoxLayout::Orientation orientation);
  ~BoxLayout() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BoxLayout);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_BOX_LAYOUT_H_

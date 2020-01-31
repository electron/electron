// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_VIEWS_ATOM_API_RESIZE_AREA_H_
#define SHELL_BROWSER_API_VIEWS_ATOM_API_RESIZE_AREA_H_

#include "gin/handle.h"
#include "shell/browser/api/atom_api_view.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/controls/resize_area_delegate.h"

namespace electron {

namespace api {

class ResizeArea : public View, protected views::ResizeAreaDelegate {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  void OnResize(int resize_amount, bool done_resizing) override;

 private:
  ResizeArea();
  ~ResizeArea() override;

  views::ResizeArea* resize_area() const {
    return static_cast<views::ResizeArea*>(view());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ResizeArea);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_VIEWS_ATOM_API_RESIZE_AREA_H_

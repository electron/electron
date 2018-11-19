// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_VIEWS_ATOM_API_RESIZE_AREA_H_
#define ATOM_BROWSER_API_VIEWS_ATOM_API_RESIZE_AREA_H_

#include "atom/browser/api/atom_api_view.h"
#include "native_mate/handle.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/controls/resize_area_delegate.h"

namespace atom {

namespace api {

class ResizeArea : public View, protected views::ResizeAreaDelegate {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

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

}  // namespace atom

#endif  // ATOM_BROWSER_API_VIEWS_ATOM_API_RESIZE_AREA_H_

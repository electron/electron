// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_VIEW_H_
#define ATOM_BROWSER_API_ATOM_API_VIEW_H_

#include <memory>

#include "atom/browser/api/event_emitter.h"
#include "ui/views/view.h"

namespace atom {

namespace api {

class View : public mate::EventEmitter<View> {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  views::View* view() const { return view_; }

 protected:
  explicit View(views::View* view);
  View();
  ~View() override;

  // Should delete the |view_| in destructor.
  void set_delete_view(bool should) { delete_view_ = should; }

 private:
  bool delete_view_ = true;
  views::View* view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(View);
};

}  // namespace api

}  // namespace atom

namespace mate {

template <>
struct Converter<views::View*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::View** out) {
    atom::api::View* view;
    if (!Converter<atom::api::View*>::FromV8(isolate, val, &view))
      return false;
    *out = view->view();
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_ATOM_API_VIEW_H_

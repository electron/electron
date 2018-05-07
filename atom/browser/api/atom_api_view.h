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

 protected:
  View();
  ~View() override;

 private:
  std::unique_ptr<views::View> view_;

  DISALLOW_COPY_AND_ASSIGN(View);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_VIEW_H_

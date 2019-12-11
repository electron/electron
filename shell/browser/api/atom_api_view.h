// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_VIEW_H_
#define SHELL_BROWSER_API_ATOM_API_VIEW_H_

#include <memory>
#include <vector>

#include "electron/buildflags/buildflags.h"
#include "gin/handle.h"
#include "shell/browser/api/views/atom_api_layout_manager.h"
#include "ui/views/view.h"

namespace electron {

namespace api {

class View : public gin_helper::TrackableObject<View> {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if BUILDFLAG(ENABLE_VIEW_API)
  void SetLayoutManager(gin::Handle<LayoutManager> layout_manager);
  void AddChildView(gin::Handle<View> view);
  void AddChildViewAt(gin::Handle<View> view, size_t index);
#endif

  views::View* view() const { return view_; }

 protected:
  explicit View(views::View* view);
  View();
  ~View() override;

  // Should delete the |view_| in destructor.
  void set_delete_view(bool should) { delete_view_ = should; }

 private:
  v8::Global<v8::Object> layout_manager_;
  std::vector<v8::Global<v8::Object>> child_views_;

  bool delete_view_ = true;
  views::View* view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(View);
};

}  // namespace api

}  // namespace electron

namespace gin {

template <>
struct Converter<views::View*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::View** out) {
    electron::api::View* view;
    if (!Converter<electron::api::View*>::FromV8(isolate, val, &view))
      return false;
    *out = view->view();
    return true;
  }
};

}  // namespace gin

#endif  // SHELL_BROWSER_API_ATOM_API_VIEW_H_

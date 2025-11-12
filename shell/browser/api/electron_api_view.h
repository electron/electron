// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "v8/include/v8-value.h"

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {

class View : public gin_helper::EventEmitter<View>,
             private views::ViewObserver {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);
  static gin_helper::Handle<View> Create(v8::Isolate* isolate);

  // Return the cached constructor function.
  static v8::Local<v8::Function> GetConstructor(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void AddChildViewAt(gin_helper::Handle<View> child,
                      std::optional<size_t> index);
  void RemoveChildView(gin_helper::Handle<View> child);

  void SetBounds(const gfx::Rect& bounds, gin::Arguments* args);
  gfx::Rect GetBounds() const;
  void SetLayout(v8::Isolate* isolate, v8::Local<v8::Object> value);
  std::vector<v8::Local<v8::Value>> GetChildren();
  void SetBackgroundColor(std::optional<WrappedSkColor> color);
  void SetBorderRadius(int radius);
  void SetVisible(bool visible);
  bool GetVisible() const;

  views::View* view() const { return view_; }
  std::optional<int> border_radius() const { return border_radius_; }

  // disable copy
  View(const View&) = delete;
  View& operator=(const View&) = delete;

 protected:
  explicit View(views::View* view);
  View();
  ~View() override;

  // Should delete the |view_| in destructor.
  void set_delete_view(bool should) { delete_view_ = should; }

 private:
  using ChildPair = std::pair<raw_ptr<views::View>, v8::Global<v8::Object>>;

  // views::ViewObserver
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;
  void OnChildViewRemoved(views::View* observed_view,
                          views::View* child) override;

  ui::Layer* GetLayer();
  void ApplyBorderRadius();
  void ReorderChildView(gin_helper::Handle<View> child, size_t index);

  std::vector<ChildPair> child_views_;
  std::optional<int> border_radius_;

  bool delete_view_ = true;
  raw_ptr<views::View> view_ = nullptr;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_

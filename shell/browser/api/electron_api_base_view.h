// Copyright (c) 2022 GitHub, Inc.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BASE_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BASE_VIEW_H_

#include <memory>
#include <string>

#include "shell/browser/ui/native_view.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace gin {
class Arguments;
template <typename T>
class Handle;
}  // namespace gin

namespace electron {

namespace api {

class BaseView : public gin_helper::TrackableObject<BaseView>,
                 public NativeView::Observer {
 public:
  static gin_helper::WrappableBase* New(gin_helper::ErrorThrower thrower,
                                        gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  NativeView* view() const { return view_.get(); }

  int32_t GetID() const;

  bool EnsureDetachFromParent();

  // disable copy
  BaseView(const BaseView&) = delete;
  BaseView& operator=(const BaseView&) = delete;

 protected:
  friend class ScrollView;

  // Common constructor.
  BaseView(v8::Isolate* isolate, NativeView* native_view);
  // Creating independent BaseView instance.
  BaseView(gin::Arguments* args, NativeView* native_view);
  ~BaseView() override;

  // TrackableObject:
  void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) override;

  // NativeView::Observer:
  void OnChildViewDetached(NativeView* observed_view,
                           NativeView* view) override;
  void OnViewIsDeleting(NativeView* observed_view) override;

  bool IsContainer() const;
  void SetBounds(const gfx::Rect& bounds);
  gfx::Rect GetBounds() const;
  void SetBackgroundColor(const std::string& color_name);
  v8::Local<v8::Value> GetParentView() const;
  v8::Local<v8::Value> GetParentWindow() const;

  virtual void ResetChildView(BaseView* view);
  virtual void ResetChildViews();

 private:
  scoped_refptr<NativeView> view_;

  // Reference to JS wrapper to prevent garbage collection.
  v8::Global<v8::Value> self_ref_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BASE_VIEW_H_

// Copyright (c) 2022 GitHub, Inc.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CONTAINER_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CONTAINER_VIEW_H_

#include <map>
#include <memory>
#include <vector>

#include "shell/browser/api/electron_api_base_view.h"
#include "shell/browser/ui/native_container_view.h"

namespace electron {

namespace api {

class ContainerView : public BaseView {
 public:
  static gin_helper::WrappableBase* New(gin_helper::ErrorThrower thrower,
                                        gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // disable copy
  ContainerView(const ContainerView&) = delete;
  ContainerView& operator=(const ContainerView&) = delete;

 protected:
  ContainerView(gin::Arguments* args, NativeContainerView* container);
  ~ContainerView() override;

  // BaseView:
  void ResetChildView(BaseView* view) override;
  void ResetChildViews() override;

  void AddChildView(v8::Local<v8::Value> value);
  void RemoveChildView(v8::Local<v8::Value> value);
  std::vector<v8::Local<v8::Value>> GetViews() const;

 private:
  NativeContainerView* container_;

  std::map<int32_t, v8::Global<v8::Value>> base_views_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CONTAINER_VIEW_H_

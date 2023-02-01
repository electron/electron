// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_PREVENTABLE_EVENT_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_PREVENTABLE_EVENT_H_

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/constructible.h"

namespace v8 {
class Isolate;
template <typename T>
class Local;
class Object;
class ObjectTemplate;
}  // namespace v8

namespace gin_helper::internal {

class PreventableEvent : public gin::Wrappable<PreventableEvent>,
                         public gin_helper::Constructible<PreventableEvent> {
 public:
  // gin_helper::Constructible
  static gin::Handle<PreventableEvent> New(v8::Isolate* isolate,
                                           v8::Local<v8::Object> sender);
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::ObjectTemplate> prototype);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;

  ~PreventableEvent() override;

  void PreventDefault() { default_prevented_ = true; }

  bool GetDefaultPrevented() { return default_prevented_; }
  v8::Local<v8::Object> GetSender(v8::Isolate* isolate);

 private:
  PreventableEvent(v8::Isolate* isolate, v8::Local<v8::Object> sender);

  bool default_prevented_ = false;
  v8::Global<v8::Object> sender_;
};

gin::Handle<PreventableEvent> CreateCustomEvent(v8::Isolate* isolate,
                                                v8::Local<v8::Object> sender);

}  // namespace gin_helper::internal

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_PREVENTABLE_EVENT_H_

// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_EVENT_H_
#define ELECTRON_SHELL_BROWSER_API_EVENT_H_

#include "electron/shell/common/api/api.mojom.h"
#include "gin/handle.h"
#include "gin/wrappable.h"

namespace gin_helper {

class Event : public gin::Wrappable<Event> {
 public:
  using InvokeCallback = electron::mojom::ElectronApiIPC::InvokeCallback;

  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<Event> Create(v8::Isolate* isolate);

  // Pass the callback to be invoked.
  void SetCallback(InvokeCallback callback);

  // event.PreventDefault().
  void PreventDefault(v8::Isolate* isolate);

  // event.sendReply(value), used for replying to synchronous messages and
  // `invoke` calls.
  bool SendReply(v8::Isolate* isolate, v8::Local<v8::Value> result);

  // disable copy
  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;

 protected:
  Event();
  ~Event() override;

  // gin::Wrappable:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  // Replyer for the synchronous messages.
  InvokeCallback callback_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_BROWSER_API_EVENT_H_

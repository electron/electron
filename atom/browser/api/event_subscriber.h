// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_
#define ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "v8/include/v8.h"

namespace mate {

class Arguments;

class EventSubscriber {
 public:
  using EventCallback = base::Callback<void(mate::Arguments* args)>;

  EventSubscriber(v8::Isolate* isolate, v8::Local<v8::Object> emitter);
  ~EventSubscriber();

  void On(const std::string& event, EventCallback&& callback);
  void Off(const std::string& event);
  void RemoveAllListeners();
  void EventEmitted(const std::string& event, mate::Arguments* args);

 private:
  using JSHandlersMap = std::map<std::string, v8::Global<v8::Value>>;

  void RemoveListener(JSHandlersMap::iterator it);

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;
  JSHandlersMap js_handlers_;
  std::map<std::string, EventCallback> callbacks_;
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

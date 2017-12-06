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
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "v8/include/v8.h"

namespace atom {
class URLRequestStreamJob;
}

namespace mate {

class Arguments;

class EventSubscriber {
 public:
  using EventCallback = base::Callback<void(mate::Arguments* args)>;

  EventSubscriber(v8::Isolate* isolate,
                  v8::Local<v8::Object> emitter,
                  base::WeakPtr<atom::URLRequestStreamJob> url_job);
  ~EventSubscriber();

 private:
  using JSHandlersMap = std::map<std::string, v8::Global<v8::Value>>;

  void On(const std::string& event, EventCallback&& callback);  // NOLINT
  void Off(const std::string& event);

  void OnData(mate::Arguments* args);
  void OnEnd(mate::Arguments* args);
  void OnError(mate::Arguments* args);

  void RemoveAllListeners();
  void RemoveListener(JSHandlersMap::iterator it);
  void EventEmitted(const std::string& event, mate::Arguments* args);

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;
  base::WeakPtr<atom::URLRequestStreamJob> url_job_;

  JSHandlersMap js_handlers_;
  std::map<std::string, EventCallback> callbacks_;

  base::WeakPtrFactory<EventSubscriber> weak_factory_;
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

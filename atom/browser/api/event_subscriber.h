// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_
#define ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "atom/common/api/event_emitter_caller.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/arguments.h"

namespace mate {

namespace internal {

class EventSubscriberBase {
 public:
  EventSubscriberBase(v8::Isolate* isolate, v8::Local<v8::Object> emitter);
  virtual ~EventSubscriberBase();
  virtual void EventEmitted(const std::string& event_name,
                            mate::Arguments* args) = 0;

 protected:
  void On(const std::string& event_name);
  void Off(const std::string& event_name);
  void RemoveAllListeners();

 private:
  std::map<std::string, v8::Global<v8::Value>>::iterator RemoveListener(
      std::map<std::string, v8::Global<v8::Value>>::iterator it);

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;
  std::map<std::string, v8::Global<v8::Value>> js_handlers_;

  DISALLOW_COPY_AND_ASSIGN(EventSubscriberBase);
};

}  // namespace internal

template <typename HandlerType>
class EventSubscriber : internal::EventSubscriberBase {
 public:
  using EventCallback = base::Callback<void(mate::Arguments* args)>;

  EventSubscriber(v8::Isolate* isolate,
                  v8::Local<v8::Object> emitter)
      : EventSubscriberBase(isolate, emitter) {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
  }

  ~EventSubscriber() {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
  }

  void On(const std::string& event, EventCallback&& callback) {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
    EventSubscriberBase::On(event);
    callbacks_.insert(std::make_pair(event, callback));
  }

  void Off(const std::string& event) {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
    EventSubscriberBase::Off(event);
    DCHECK(callbacks_.find(event) != callbacks_.end());
    callbacks_.erase(callbacks_.find(event));
  }

  void RemoveAllListeners() {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
    EventSubscriberBase::RemoveAllListeners();
    callbacks_.clear();
  }

 private:
  void EventEmitted(const std::string& event_name,
                    mate::Arguments* args) override {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
    auto it = callbacks_.find(event_name);
    if (it != callbacks_.end()) {
      it->second.Run(args);
    }
  }

  std::map<std::string, EventCallback> callbacks_;
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

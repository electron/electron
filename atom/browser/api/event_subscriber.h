// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_
#define ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

#include <map>
#include <string>

#include "atom/common/api/event_emitter_caller.h"
#include "base/synchronization/lock.h"
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
  using EventCallback = void (HandlerType::*)(mate::Arguments* args);
  // Alias to unique_ptr with deleter.
  using unique_ptr = std::unique_ptr<EventSubscriber<HandlerType>,
                                     void (*)(EventSubscriber<HandlerType>*)>;
  // EventSubscriber should only be created/deleted in the main thread since it
  // communicates with the V8 engine. This smart pointer makes it simpler to
  // bind the lifetime of EventSubscriber with a class whose lifetime is managed
  // by a non-UI thread.
  class SafePtr : public unique_ptr {
   public:
    SafePtr() : SafePtr(nullptr) {}
    explicit SafePtr(EventSubscriber<HandlerType>* ptr)
        : unique_ptr(ptr, Deleter) {}

   private:
    // Custom deleter that schedules destructor invocation to the main thread.
    static void Deleter(EventSubscriber<HandlerType>* ptr) {
      DCHECK(
          !::content::BrowserThread::CurrentlyOn(::content::BrowserThread::UI));
      DCHECK(ptr);
      // Acquire handler lock and reset handler_ to ensure that any new events
      // emitted will be ignored after this function returns
      base::AutoLock auto_lock(ptr->handler_lock_);
      ptr->handler_ = nullptr;
      content::BrowserThread::PostTask(
          content::BrowserThread::UI, FROM_HERE,
          base::BindOnce(
              [](EventSubscriber<HandlerType>* subscriber) {
                {
                  // It is possible that this function will execute in the UI
                  // thread before the outer function has returned and destroyed
                  // its auto_lock. We need to acquire the lock before deleting
                  // or risk a crash.
                  base::AutoLock auto_lock(subscriber->handler_lock_);
                }
                delete subscriber;
              },
              ptr));
    }
  };

  EventSubscriber(HandlerType* handler,
                  v8::Isolate* isolate,
                  v8::Local<v8::Object> emitter)
      : EventSubscriberBase(isolate, emitter), handler_(handler) {
    DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
  }

  void On(const std::string& event, EventCallback callback) {
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
    base::AutoLock auto_lock(handler_lock_);
    if (!handler_) {
      // handler_ was probably destroyed by another thread and we should not
      // access it.
      return;
    }
    auto it = callbacks_.find(event_name);
    if (it != callbacks_.end()) {
      auto method = it->second;
      (handler_->*method)(args);
    }
  }

  HandlerType* handler_;
  base::Lock handler_lock_;
  std::map<std::string, EventCallback> callbacks_;
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_EVENT_SUBSCRIBER_H_

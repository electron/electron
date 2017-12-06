// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/browser/api/event_subscriber.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/callback.h"

namespace mate {

EventSubscriber::EventSubscriber(v8::Isolate* isolate,
                                 v8::Local<v8::Object> emitter)
    : isolate_(isolate),
      emitter_(isolate, emitter),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

EventSubscriber::~EventSubscriber() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RemoveAllListeners();
}

void EventSubscriber::On(const std::string& event, EventCallback&& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  callbacks_[event] = callback;

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  // emitter.on(event, EventEmitted)
  auto fn = CallbackToV8(isolate_, base::Bind(&EventSubscriber::EventEmitted,
                                              weak_factory_.GetWeakPtr(),
                                              event));
  js_handlers_[event] = v8::Global<v8::Value>(isolate_, fn);
  internal::ValueVector args = { StringToV8(isolate_, event), fn };
  internal::CallMethodWithArgs(isolate_, emitter_.Get(isolate_), "on", &args);
}

void EventSubscriber::Off(const std::string& event) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(callbacks_.find(event) != callbacks_.end());
  callbacks_.erase(event);

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto js_handler = js_handlers_.find(event);
  DCHECK(js_handler != js_handlers_.end());
  RemoveListener(js_handler);
}

void EventSubscriber::RemoveAllListeners() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  callbacks_.clear();

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  while (!js_handlers_.empty()) {
    RemoveListener(js_handlers_.begin());
  }
}

void EventSubscriber::RemoveListener(JSHandlersMap::iterator it) {
  internal::ValueVector args = { StringToV8(isolate_, it->first),
                                 it->second.Get(isolate_) };
  internal::CallMethodWithArgs(isolate_, emitter_.Get(isolate_),
                               "removeListener", &args);
  js_handlers_.erase(it);
}

void EventSubscriber::EventEmitted(const std::string& event,
                                   mate::Arguments* args) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto it = callbacks_.find(event);
  if (it != callbacks_.end())
    it->second.Run(args);
}

}  // namespace mate

// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include <string>

#include "atom/browser/api/event_subscriber.h"
#include "atom/common/native_mate_converters/callback.h"

namespace {

// A FunctionTemplate lifetime is bound to the v8 context, so it can be safely
// stored as a global here since there's only one for the main process.
v8::Global<v8::FunctionTemplate> g_cached_template;

struct JSHandlerData {
  JSHandlerData(v8::Isolate* isolate,
                mate::internal::EventSubscriberBase* subscriber)
      : handle_(isolate, v8::External::New(isolate, this)),
        subscriber_(subscriber) {
    handle_.SetWeak(this, GC, v8::WeakCallbackType::kFinalizer);
  }

  static void GC(const v8::WeakCallbackInfo<JSHandlerData>& data) {
    delete data.GetParameter();
  }

  v8::Global<v8::External> handle_;
  mate::internal::EventSubscriberBase* subscriber_;
};

void InvokeCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Locker locker(info.GetIsolate());
  v8::HandleScope handle_scope(info.GetIsolate());
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  v8::Context::Scope context_scope(context);
  mate::Arguments args(info);
  v8::Local<v8::Value> handler, event;
  args.GetNext(&handler);
  args.GetNext(&event);
  DCHECK(handler->IsExternal());
  DCHECK(event->IsString());
  JSHandlerData* handler_data = static_cast<JSHandlerData*>(
      v8::Local<v8::External>::Cast(handler)->Value());
  handler_data->subscriber_->EventEmitted(mate::V8ToString(event), &args);
}

}  // namespace

namespace mate {

namespace internal {

EventSubscriberBase::EventSubscriberBase(v8::Isolate* isolate,
                                         v8::Local<v8::Object> emitter)
    : isolate_(isolate), emitter_(isolate, emitter) {
  if (g_cached_template.IsEmpty()) {
    g_cached_template = v8::Global<v8::FunctionTemplate>(
        isolate_, v8::FunctionTemplate::New(isolate_, InvokeCallback));
  }
}

EventSubscriberBase::~EventSubscriberBase() {
  if (!isolate_) {
    return;
  }
  RemoveAllListeners();
  emitter_.Reset();
  DCHECK_EQ(js_handlers_.size(), 0u);
}

void EventSubscriberBase::On(const std::string& event_name) {
  DCHECK(js_handlers_.find(event_name) == js_handlers_.end());
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto fn_template = g_cached_template.Get(isolate_);
  auto event = mate::StringToV8(isolate_, event_name);
  auto js_handler_data = new JSHandlerData(isolate_, this);
  v8::Local<v8::Value> fn = internal::BindFunctionWith(
      isolate_, isolate_->GetCurrentContext(), fn_template->GetFunction(),
      js_handler_data->handle_.Get(isolate_), event);
  js_handlers_.insert(
      std::make_pair(event_name, v8::Global<v8::Value>(isolate_, fn)));
  internal::ValueVector converted_args = {event, fn};
  internal::CallMethodWithArgs(isolate_, emitter_.Get(isolate_), "on",
                               &converted_args);
}

void EventSubscriberBase::Off(const std::string& event_name) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto js_handler = js_handlers_.find(event_name);
  DCHECK(js_handler != js_handlers_.end());
  RemoveListener(js_handler);
}

void EventSubscriberBase::RemoveAllListeners() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  while (!js_handlers_.empty()) {
    RemoveListener(js_handlers_.begin());
  }
}

std::map<std::string, v8::Global<v8::Value>>::iterator
EventSubscriberBase::RemoveListener(
    std::map<std::string, v8::Global<v8::Value>>::iterator it) {
  internal::ValueVector args = {StringToV8(isolate_, it->first),
                                it->second.Get(isolate_)};
  internal::CallMethodWithArgs(
      isolate_, v8::Local<v8::Object>::Cast(emitter_.Get(isolate_)),
      "removeListener", &args);
  it->second.Reset();
  return js_handlers_.erase(it);
}

}  // namespace internal

}  // namespace mate

// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/browser/api/event_subscriber.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/callback.h"
#include "native_mate/native_mate/arguments.h"

namespace {

// A FunctionTemplate lifetime is bound to the v8 context, so it can be safely
// stored as a global here since there's only one for the main process.
v8::Global<v8::FunctionTemplate> g_cached_template;

struct JSHandlerData {
  JSHandlerData(v8::Isolate* isolate,
                mate::EventSubscriber* subscriber)
      : handle_(isolate, v8::External::New(isolate, this)),
        subscriber_(subscriber) {
    handle_.SetWeak(this, GC, v8::WeakCallbackType::kFinalizer);
  }

  static void GC(const v8::WeakCallbackInfo<JSHandlerData>& data) {
    delete data.GetParameter();
  }

  v8::Global<v8::External> handle_;
  mate::EventSubscriber* subscriber_;
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

EventSubscriber::EventSubscriber(v8::Isolate* isolate,
                                 v8::Local<v8::Object> emitter)
    : isolate_(isolate), emitter_(isolate, emitter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (g_cached_template.IsEmpty()) {
    g_cached_template = v8::Global<v8::FunctionTemplate>(
        isolate_, v8::FunctionTemplate::New(isolate_, InvokeCallback));
  }
}

EventSubscriber::~EventSubscriber() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RemoveAllListeners();
  emitter_.Reset();
  DCHECK_EQ(js_handlers_.size(), 0u);
}

void EventSubscriber::On(const std::string& event, EventCallback&& callback) {
  DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
  DCHECK(js_handlers_.find(event) == js_handlers_.end());
  callbacks_.insert(std::make_pair(event, callback));

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto fn_template = g_cached_template.Get(isolate_);
<<<<<<< HEAD
  auto event = mate::StringToV8(isolate_, event_name);
  auto* js_handler_data = new JSHandlerData(isolate_, this);
=======
  auto v8event = mate::StringToV8(isolate_, event);
  auto js_handler_data = new JSHandlerData(isolate_, this);
>>>>>>> Simplify EventSubscriber
  v8::Local<v8::Value> fn = internal::BindFunctionWith(
      isolate_, isolate_->GetCurrentContext(), fn_template->GetFunction(),
      js_handler_data->handle_.Get(isolate_), v8event);
  js_handlers_.insert(
      std::make_pair(event, v8::Global<v8::Value>(isolate_, fn)));
  internal::ValueVector converted_args = { v8event, fn };
  internal::CallMethodWithArgs(isolate_, emitter_.Get(isolate_), "on",
                               &converted_args);
}

void EventSubscriber::Off(const std::string& event) {
  DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
  DCHECK(callbacks_.find(event) != callbacks_.end());
  callbacks_.erase(callbacks_.find(event));

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto js_handler = js_handlers_.find(event);
  DCHECK(js_handler != js_handlers_.end());
  RemoveListener(js_handler);
}

void EventSubscriber::RemoveAllListeners() {
  DCHECK_CURRENTLY_ON(::content::BrowserThread::UI);
  callbacks_.clear();

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  while (!js_handlers_.empty()) {
    RemoveListener(js_handlers_.begin());
  }
}

void EventSubscriber::EventEmitted(const std::string& event,
                                   mate::Arguments* args) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto it = callbacks_.find(event);
  if (it != callbacks_.end())
    it->second.Run(args);
}

void EventSubscriber::RemoveListener(JSHandlersMap::iterator it) {
  internal::ValueVector args = { StringToV8(isolate_, it->first),
                                 it->second.Get(isolate_) };
  internal::CallMethodWithArgs(
      isolate_, v8::Local<v8::Object>::Cast(emitter_.Get(isolate_)),
      "removeListener", &args);
  it->second.Reset();
  js_handlers_.erase(it);
}

}  // namespace mate

// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_subscriber.h"

#include <deque>
#include <string>

#include "atom/browser/net/url_request_stream_job.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/callback.h"

#include "atom/common/node_includes.h"

namespace mate {

EventSubscriber::EventSubscriber(
    v8::Isolate* isolate,
    v8::Local<v8::Object> emitter,
    base::WeakPtr<atom::URLRequestStreamJob> url_job)
    : isolate_(isolate),
      emitter_(isolate, emitter),
      url_job_(url_job),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto weak_self = weak_factory_.GetWeakPtr();
  On("data", base::Bind(&EventSubscriber::OnData, weak_self));
  On("end", base::Bind(&EventSubscriber::OnEnd, weak_self));
  On("error", base::Bind(&EventSubscriber::OnError, weak_self));
}

EventSubscriber::~EventSubscriber() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RemoveAllListeners();
}

void EventSubscriber::On(const std::string& event, EventCallback&& callback) {  // NOLINT
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(callbacks_.find(event) == callbacks_.end());
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

void EventSubscriber::OnData(mate::Arguments* args) {
  v8::Local<v8::Value> buf;
  args->GetNext(&buf);
  if (!node::Buffer::HasInstance(buf)) {
    args->ThrowError("data must be Buffer");
    return;
  }

  // Copy the data from Buffer.
  // TODO(zcbenz): Is there a way to move the data?
  const char* data = node::Buffer::Data(buf);
  size_t length = node::Buffer::Length(buf);
  std::vector<char> buffer(length);
  buffer.assign(data, data + length);
  // Pass the data to the URLJob in IO thread.
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&atom::URLRequestStreamJob::OnData,
                 url_job_, buffer));
}

void EventSubscriber::OnEnd(mate::Arguments* args) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&atom::URLRequestStreamJob::OnEnd, url_job_));
}

void EventSubscriber::OnError(mate::Arguments* args) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&atom::URLRequestStreamJob::OnError, url_job_));
}

void EventSubscriber::RemoveAllListeners() {
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
  auto it = callbacks_.find(event);
  if (it != callbacks_.end())
    it->second.Run(args);
}

}  // namespace mate

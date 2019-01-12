// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/stream_subscriber.h"

#include <string>

#include "atom/browser/net/url_request_stream_job.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/callback.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"

#include "atom/common/node_includes.h"

namespace mate {

StreamSubscriber::StreamSubscriber(
    v8::Isolate* isolate,
    v8::Local<v8::Object> emitter,
    base::WeakPtr<atom::URLRequestStreamJob> url_job)
    : isolate_(isolate),
      emitter_(isolate, emitter),
      url_job_(url_job),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto weak_self = weak_factory_.GetWeakPtr();
  On("data", base::Bind(&StreamSubscriber::OnData, weak_self));
  On("end", base::Bind(&StreamSubscriber::OnEnd, weak_self));
  On("error", base::Bind(&StreamSubscriber::OnError, weak_self));
}

StreamSubscriber::~StreamSubscriber() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  RemoveAllListeners();
}

void StreamSubscriber::On(const std::string& event,
                          EventCallback&& callback) {  // NOLINT
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(js_handlers_.find(event) == js_handlers_.end());

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  // emitter.on(event, EventEmitted)
  auto fn = CallbackToV8(isolate_, callback);
  js_handlers_[event] = v8::Global<v8::Value>(isolate_, fn);
  internal::ValueVector args = {StringToV8(isolate_, event), fn};
  internal::CallMethodWithArgs(isolate_, emitter_.Get(isolate_), "on", &args);
}

void StreamSubscriber::Off(const std::string& event) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(js_handlers_.find(event) != js_handlers_.end());

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  auto js_handler = js_handlers_.find(event);
  DCHECK(js_handler != js_handlers_.end());
  RemoveListener(js_handler);
}

void StreamSubscriber::OnData(mate::Arguments* args) {
  v8::Local<v8::Value> buf;
  args->GetNext(&buf);
  if (!node::Buffer::HasInstance(buf)) {
    args->ThrowError("data must be Buffer");
    return;
  }

  const char* data = node::Buffer::Data(buf);
  size_t length = node::Buffer::Length(buf);
  if (length == 0)
    return;

  // Pass the data to the URLJob in IO thread.
  std::vector<char> buffer(data, data + length);
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::Bind(&atom::URLRequestStreamJob::OnData,
                                      url_job_, base::Passed(&buffer)));
}

void StreamSubscriber::OnEnd(mate::Arguments* args) {
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::Bind(&atom::URLRequestStreamJob::OnEnd, url_job_));
}

void StreamSubscriber::OnError(mate::Arguments* args) {
  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::Bind(&atom::URLRequestStreamJob::OnError,
                                      url_job_, net::ERR_FAILED));
}

void StreamSubscriber::RemoveAllListeners() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  while (!js_handlers_.empty()) {
    RemoveListener(js_handlers_.begin());
  }
}

void StreamSubscriber::RemoveListener(JSHandlersMap::iterator it) {
  internal::ValueVector args = {StringToV8(isolate_, it->first),
                                it->second.Get(isolate_)};
  internal::CallMethodWithArgs(isolate_, emitter_.Get(isolate_),
                               "removeListener", &args);
  js_handlers_.erase(it);
}

}  // namespace mate

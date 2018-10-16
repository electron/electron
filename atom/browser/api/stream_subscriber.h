// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_STREAM_SUBSCRIBER_H_
#define ATOM_BROWSER_API_STREAM_SUBSCRIBER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "v8/include/v8.h"

namespace atom {
class URLRequestStreamJob;
}

namespace mate {

class Arguments;

class StreamSubscriber {
 public:
  StreamSubscriber(v8::Isolate* isolate,
                   v8::Local<v8::Object> emitter,
                   base::WeakPtr<atom::URLRequestStreamJob> url_job);
  ~StreamSubscriber();

 private:
  using JSHandlersMap = std::map<std::string, v8::Global<v8::Value>>;
  using EventCallback = base::Callback<void(mate::Arguments* args)>;

  void On(const std::string& event, EventCallback&& callback);  // NOLINT
  void Off(const std::string& event);

  void OnData(mate::Arguments* args);
  void OnEnd(mate::Arguments* args);
  void OnError(mate::Arguments* args);

  void RemoveAllListeners();
  void RemoveListener(JSHandlersMap::iterator it);

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;
  base::WeakPtr<atom::URLRequestStreamJob> url_job_;

  JSHandlersMap js_handlers_;

  base::WeakPtrFactory<StreamSubscriber> weak_factory_;
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_STREAM_SUBSCRIBER_H_

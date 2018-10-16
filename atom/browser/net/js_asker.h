// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_JS_ASKER_H_
#define ATOM_BROWSER_NET_JS_ASKER_H_

#include <memory>

#include "base/callback.h"
#include "base/values.h"
#include "native_mate/arguments.h"
#include "net/url_request/url_request_context_getter.h"
#include "v8/include/v8.h"

namespace atom {

using JavaScriptHandler =
    base::Callback<void(const base::DictionaryValue&, v8::Local<v8::Value>)>;
using BeforeStartCallback = base::Callback<void(mate::Arguments* args)>;

class JsAsker {
 public:
  JsAsker();
  ~JsAsker();

  // Called by |CustomProtocolHandler| to store handler related information.
  void SetHandlerInfo(v8::Isolate* isolate,
                      net::URLRequestContextGetter* request_context_getter,
                      const JavaScriptHandler& handler);

  // Ask handler for options in UI thread.
  static void AskForOptions(
      v8::Isolate* isolate,
      const JavaScriptHandler& handler,
      std::unique_ptr<base::DictionaryValue> request_details,
      const BeforeStartCallback& before_start);

  // Test whether the |options| means an error.
  static bool IsErrorOptions(base::Value* value, int* error);

  net::URLRequestContextGetter* request_context_getter() const {
    return request_context_getter_;
  }
  v8::Isolate* isolate() { return isolate_; }
  JavaScriptHandler handler() { return handler_; }

 private:
  v8::Isolate* isolate_;
  net::URLRequestContextGetter* request_context_getter_;
  JavaScriptHandler handler_;

  DISALLOW_COPY_AND_ASSIGN(JsAsker);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_JS_ASKER_H_

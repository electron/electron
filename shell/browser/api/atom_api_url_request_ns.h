// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_

#include "shell/browser/api/event_emitter.h"

namespace electron {

namespace api {

class URLRequestNS : public mate::EventEmitter<URLRequestNS> {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  URLRequestNS(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);
  ~URLRequestNS() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLRequestNS);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_

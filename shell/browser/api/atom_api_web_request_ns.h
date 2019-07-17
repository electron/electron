// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "shell/browser/api/trackable_object.h"

namespace electron {

class AtomBrowserContext;

namespace api {

class WebRequestNS : public mate::TrackableObject<WebRequestNS> {
 public:
  static mate::Handle<WebRequestNS> Create(v8::Isolate* isolate,
                                           AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 private:
  WebRequestNS(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~WebRequestNS() override;
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

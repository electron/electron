// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"

namespace electron {

class AtomBrowserContext;

namespace api {

class WebRequestNS : public gin::Wrappable<WebRequestNS> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<WebRequestNS> Create(v8::Isolate* isolate,
                                          AtomBrowserContext* browser_context);

  // gin::Wrappable:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  WebRequestNS(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~WebRequestNS() override;
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

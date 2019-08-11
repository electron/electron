// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

#include "base/values.h"
#include "gin/arguments.h"
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

  enum SimpleEvent {
    kOnSendHeaders,
    kOnBeforeRedirect,
    kOnResponseStarted,
    kOnCompleted,
    kOnErrorOccurred,
  };
  enum ResponseEvent {
    kOnBeforeRequest,
    kOnBeforeSendHeaders,
    kOnHeadersReceived,
  };

  using SimpleListener = base::RepeatingCallback<void(const base::Value&)>;
  using ResponseCallback = base::OnceCallback<void(const base::Value&)>;
  using ResponseListener =
      base::RepeatingCallback<void(const base::Value&, ResponseCallback)>;

  template <SimpleEvent event>
  void SetSimpleListener(gin::Arguments* args);
  template <ResponseEvent event>
  void SetResponseListener(gin::Arguments* args);
  template <typename Listener, typename Event>
  void SetListener(Event event, gin::Arguments* args);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

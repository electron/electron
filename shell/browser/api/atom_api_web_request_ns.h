// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

#include <map>
#include <set>

#include "base/values.h"
#include "extensions/common/url_pattern.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "shell/browser/net/proxying_url_loader_factory.h"

namespace content {
class BrowserContext;
}

namespace electron {

namespace api {

class WebRequestNS : public gin::Wrappable<WebRequestNS>, public WebRequestAPI {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<WebRequestNS> FromOrCreate(
      v8::Isolate* isolate,
      content::BrowserContext* browser_context);
  static WebRequestNS* From(content::BrowserContext* browser_context);

  // gin::Wrappable:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  WebRequestNS(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~WebRequestNS() override;

  // WebRequestAPI:
  int OnBeforeRequest(net::CompletionOnceCallback callback,
                      GURL* new_url) override;
  int OnBeforeSendHeaders(BeforeSendHeadersCallback callback,
                          net::HttpRequestHeaders* headers) override;
  int OnHeadersReceived(
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnSendHeaders(const net::HttpRequestHeaders& headers) override;
  void OnBeforeRedirect(const GURL& new_location) override;
  void OnResponseStarted() override;
  void OnErrorOccurred(int net_error) override;
  void OnCompleted(int net_error) override;
  void OnResponseStarted(int net_error) override;

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
  template <typename Listener, typename Listeners, typename Event>
  void SetListener(Event event, Listeners* listeners, gin::Arguments* args);

  struct SimpleListenerInfo {
    std::set<URLPattern> url_patterns;
    SimpleListener listener;

    SimpleListenerInfo(std::set<URLPattern>, SimpleListener);
    SimpleListenerInfo();
    ~SimpleListenerInfo();
  };

  struct ResponseListenerInfo {
    std::set<URLPattern> url_patterns;
    ResponseListener listener;

    ResponseListenerInfo(std::set<URLPattern>, ResponseListener);
    ResponseListenerInfo();
    ~ResponseListenerInfo();
  };

  std::map<SimpleEvent, SimpleListenerInfo> simple_listeners_;
  std::map<ResponseEvent, ResponseListenerInfo> response_listeners_;
  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_WEB_REQUEST_NS_H_

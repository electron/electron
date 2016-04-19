// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_
#define ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_

#include <map>
#include <set>
#include <string>

#include "brightray/browser/network_delegate.h"
#include "base/callback.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "extensions/common/url_pattern.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "content/public/browser/resource_request_info.h"

namespace extensions {
class URLPattern;
}

namespace atom {

using URLPatterns = std::set<extensions::URLPattern>;

const char* ResourceTypeToString(content::ResourceType type);

class AtomNetworkDelegate : public brightray::NetworkDelegate {
 public:
  using ResponseCallback = base::Callback<void(const base::DictionaryValue&)>;
  using SimpleListener = base::Callback<void(const base::DictionaryValue&)>;
  using ResponseListener = base::Callback<void(const base::DictionaryValue&,
                                               const ResponseCallback&)>;

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

  struct SimpleListenerInfo {
    URLPatterns url_patterns;
    SimpleListener listener;
  };

  struct ResponseListenerInfo {
    URLPatterns url_patterns;
    ResponseListener listener;
  };

  AtomNetworkDelegate();
  ~AtomNetworkDelegate() override;

  void SetSimpleListenerInIO(SimpleEvent type,
                             const URLPatterns& patterns,
                             const SimpleListener& callback);
  void SetResponseListenerInIO(ResponseEvent type,
                               const URLPatterns& patterns,
                               const ResponseListener& callback);

  void SetDevToolsNetworkEmulationClientId(const std::string& client_id);

 protected:
  // net::NetworkDelegate:
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  int OnBeforeSendHeaders(net::URLRequest* request,
                          const net::CompletionCallback& callback,
                          net::HttpRequestHeaders* headers) override;
  void OnSendHeaders(net::URLRequest* request,
                     const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;
  void OnResponseStarted(net::URLRequest* request) override;
  void OnCompleted(net::URLRequest* request, bool started) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;

 private:
  void OnErrorOccurred(net::URLRequest* request, bool started);

  template<typename...Args>
  void HandleSimpleEvent(SimpleEvent type,
                         net::URLRequest* request,
                         Args... args);
  template<typename Out, typename... Args>
  int HandleResponseEvent(ResponseEvent type,
                          net::URLRequest* request,
                          const net::CompletionCallback& callback,
                          Out out,
                          Args... args);

  // Deal with the results of Listener.
  template<typename T>
  void OnListenerResultInIO(
      uint64_t id, T out, scoped_ptr<base::DictionaryValue> response);
  template<typename T>
  void OnListenerResultInUI(
      uint64_t id, T out, const base::DictionaryValue& response);

  std::map<SimpleEvent, SimpleListenerInfo> simple_listeners_;
  std::map<ResponseEvent, ResponseListenerInfo> response_listeners_;
  std::map<uint64_t, net::CompletionCallback> callbacks_;

  base::Lock lock_;
  // Client id for devtools network emulation.
  std::string client_id_;

  DISALLOW_COPY_AND_ASSIGN(AtomNetworkDelegate);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_

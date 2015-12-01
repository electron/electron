// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_
#define ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_

#include <map>
#include <string>

#include "brightray/browser/network_delegate.h"
#include "base/callback.h"
#include "base/values.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace atom {

class AtomNetworkDelegate : public brightray::NetworkDelegate {
 public:
  enum EventTypes {
    kInvalidEvent = 0,
    kOnBeforeRequest = 1 << 0,
    kOnBeforeSendHeaders = 1 << 1,
    kOnSendHeaders = 1 << 2,
    kOnHeadersReceived = 1 << 3,
    kOnBeforeRedirect = 1 << 4,
    kOnResponseStarted = 1 << 5,
    kOnErrorOccurred = 1 << 6,
    kOnCompleted = 1 << 7,
  };

  struct BlockingResponse {
    BlockingResponse() {}
    ~BlockingResponse() {}

    bool cancel;
    GURL redirectURL;
    net::HttpRequestHeaders requestHeaders;
    scoped_refptr<net::HttpResponseHeaders> responseHeaders;
  };

  using Listener =
      base::Callback<BlockingResponse(const base::DictionaryValue*)>;

  AtomNetworkDelegate();
  ~AtomNetworkDelegate() override;

  void SetListenerInIO(EventTypes type,
                       const base::DictionaryValue* filter,
                       const Listener& callback);

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

 private:
  struct ListenerInfo {
    ListenerInfo() {}
    ~ListenerInfo() {}

    AtomNetworkDelegate::Listener callback;
  };

  static std::map<EventTypes, ListenerInfo> event_listener_map_;

  DISALLOW_COPY_AND_ASSIGN(AtomNetworkDelegate);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_NETWORK_DELEGATE_H_

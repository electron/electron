// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_REQUEST_CONTEXT_DELEGATE_H_
#define ATOM_BROWSER_REQUEST_CONTEXT_DELEGATE_H_

#include <string>
#include <vector>

#include "base/callback_list.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "brightray/browser/url_request_context_getter.h"

namespace atom {

struct CookieDetails;

class RequestContextDelegate
    : public brightray::URLRequestContextGetter::Delegate {
 public:
  explicit RequestContextDelegate(bool use_cache);
  ~RequestContextDelegate() override;

  // Register callbacks that needs to notified on any cookie store changes.
  std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
  RegisterCookieChangeCallback(
      const base::Callback<void(const CookieDetails*)>& cb);

 protected:
  std::unique_ptr<net::NetworkDelegate> CreateNetworkDelegate() override;
  std::unique_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      net::URLRequestContext* url_request_context,
      content::ProtocolHandlerMap* protocol_handlers) override;
  net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
      const base::FilePath& base_path) override;
  std::unique_ptr<net::CertVerifier> CreateCertVerifier(
      brightray::RequireCTDelegate* ct_delegate) override;
  void GetCookieableSchemes(std::vector<std::string>* cookie_schemes) override;
  void OnCookieChanged(const net::CanonicalCookie& cookie,
                       net::CookieChangeCause cause) override;

 private:
  void NotifyCookieChange(const net::CanonicalCookie& cookie,
                          net::CookieChangeCause cause);

  base::CallbackList<void(const CookieDetails*)> cookie_change_sub_list_;
  bool use_cache_ = true;

  base::WeakPtrFactory<RequestContextDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RequestContextDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_REQUEST_CONTEXT_DELEGATE_H_

// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_access_token_store.h"

#include <string>
#include <utility>

#include "atom/common/google_api_key.h"
#include "base/environment.h"
#include "content/public/browser/browser_thread.h"
#include "device/geolocation/geolocation_provider.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserThread;

namespace atom {

namespace internal {

class GeoURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  net::URLRequestContext* GetURLRequestContext() override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    if (!url_request_context_.get()) {
      net::URLRequestContextBuilder builder;
      builder.set_proxy_config_service(
          net::ProxyService::CreateSystemProxyConfigService(
              BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)));
      url_request_context_ = builder.Build();
    }
    return url_request_context_.get();
  }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
  }

 private:
  friend class atom::AtomAccessTokenStore;

  GeoURLRequestContextGetter() {}
  ~GeoURLRequestContextGetter() override {}

  std::unique_ptr<net::URLRequestContext> url_request_context_;
  DISALLOW_COPY_AND_ASSIGN(GeoURLRequestContextGetter);
};

}  // namespace internal

AtomAccessTokenStore::AtomAccessTokenStore()
    : request_context_getter_(new internal::GeoURLRequestContextGetter) {
}

AtomAccessTokenStore::~AtomAccessTokenStore() {
}

void AtomAccessTokenStore::LoadAccessTokens(
    const LoadAccessTokensCallback& callback) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string api_key;
  if (!env->GetVar("GOOGLE_API_KEY", &api_key))
    api_key = GOOGLEAPIS_API_KEY;
  // Equivalent to access_token_map[kGeolocationProviderURL].
  // Somehow base::string16 is causing compilation errors when used in a pair
  // of std::map on Linux, this can work around it.
  device::AccessTokenStore::AccessTokenMap access_token_map;
  std::pair<GURL, base::string16> token_pair;
  token_pair.first = GURL(GOOGLEAPIS_ENDPOINT + api_key);
  access_token_map.insert(token_pair);

  callback.Run(access_token_map, request_context_getter_.get());
}

void AtomAccessTokenStore::SaveAccessToken(const GURL& server_url,
                                           const base::string16& access_token) {
}

}  // namespace atom

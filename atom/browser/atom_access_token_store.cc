// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_access_token_store.h"

#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/common/google_api_key.h"
#include "base/environment.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/geolocation_provider.h"

namespace atom {

AtomAccessTokenStore::AtomAccessTokenStore() {
  content::GeolocationProvider::GetInstance()->UserDidOptIntoLocationServices();
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (!env->GetVar("GOOGLE_API_ENDPOINT", &endpoint_))
    endpoint_ = GOOGLEAPIS_ENDPOINT;

  if (!env->GetVar("GOOGLE_API_KEY", &api_key_))
    api_key_ = GOOGLEAPIS_API_KEY;
}

AtomAccessTokenStore::~AtomAccessTokenStore() {
}

void AtomAccessTokenStore::LoadAccessTokens(
    const LoadAccessTokensCallback& callback) {
  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&AtomAccessTokenStore::GetRequestContextOnUIThread, this),
      base::Bind(&AtomAccessTokenStore::RespondOnOriginatingThread,
                 this, callback));
}

void AtomAccessTokenStore::SaveAccessToken(const GURL& server_url,
                                           const base::string16& access_token) {
}

void AtomAccessTokenStore::GetRequestContextOnUIThread() {
  auto browser_context = AtomBrowserContext::From("", false);
  request_context_getter_ = browser_context->GetRequestContext();
}

void AtomAccessTokenStore::RespondOnOriginatingThread(
    const LoadAccessTokensCallback& callback) {
  // Equivelent to access_token_map[kGeolocationProviderURL].
  // Somehow base::string16 is causing compilation errors when used in a pair
  // of std::map on Linux, this can work around it.
  AccessTokenMap access_token_map;
  std::pair<GURL, base::string16> token_pair;
  token_pair.first = GURL(endpoint_ + api_key_);
  access_token_map.insert(token_pair);

  callback.Run(access_token_map, request_context_getter_.get());
  request_context_getter_ = nullptr;
}

}  // namespace atom

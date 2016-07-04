// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_access_token_store.h"

#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/common/google_api_key.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/geolocation_provider.h"

namespace atom {

namespace {

// Notice that we just combined the api key with the url together here, because
// if we use the standard {url: key} format Chromium would override our key with
// the predefined one in common.gypi of libchromiumcontent, which is empty.
const char* kGeolocationProviderURL =
    "https://www.googleapis.com/geolocation/v1/geolocate?key="
    GOOGLEAPIS_API_KEY;

}  // namespace

AtomAccessTokenStore::AtomAccessTokenStore() {
  LOG(ERROR) << "AtomAccessTokenStore";
  content::GeolocationProvider::GetInstance()->UserDidOptIntoLocationServices();
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
  auto browser_context = brightray::BrowserContext::From("", false);
  request_context_getter_ = browser_context->url_request_context_getter();
}

void AtomAccessTokenStore::RespondOnOriginatingThread(
    const LoadAccessTokensCallback& callback) {
  // Equivelent to access_token_map[kGeolocationProviderURL].
  // Somehow base::string16 is causing compilation errors when used in a pair
  // of std::map on Linux, this can work around it.
  AccessTokenMap access_token_map;
  std::pair<GURL, base::string16> token_pair;
  token_pair.first = GURL(kGeolocationProviderURL);
  access_token_map.insert(token_pair);

  callback.Run(access_token_map, request_context_getter_.get());
  request_context_getter_ = nullptr;
}

}  // namespace atom

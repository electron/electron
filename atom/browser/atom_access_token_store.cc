// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_access_token_store.h"

#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/common/google_api_key.h"
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
  content::GeolocationProvider::GetInstance()->UserDidOptIntoLocationServices();
}

AtomAccessTokenStore::~AtomAccessTokenStore() {
}

void AtomAccessTokenStore::LoadAccessTokens(
    const LoadAccessTokensCallback& callback) {
  AccessTokenMap access_token_map;

  // Equivelent to access_token_map[kGeolocationProviderURL].
  // Somehow base::string16 is causing compilation errors when used in a pair
  // of std::map on Linux, this can work around it.
  std::pair<GURL, base::string16> token_pair;
  token_pair.first = GURL(kGeolocationProviderURL);
  access_token_map.insert(token_pair);

  auto browser_context = AtomBrowserMainParts::Get()->browser_context();
  callback.Run(access_token_map, browser_context->url_request_context_getter());
}

void AtomAccessTokenStore::SaveAccessToken(const GURL& server_url,
                                           const base::string16& access_token) {
}

}  // namespace atom

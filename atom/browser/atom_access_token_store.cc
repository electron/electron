// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_access_token_store.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_url_request_context_getter.h"

#ifndef GOOGLEAPIS_API_KEY
#define GOOGLEAPIS_API_KEY "AIzaSyAQfxPJiounkhOjODEO5ZieffeBv6yft2Q"
#endif

namespace atom {

namespace {

// Notice that we just combined the api key with the url together here, because
// if we use the standard {url: key} format Chromium would override our key with
// the predefined one in common.gypi of libchromiumcontent, which is empty.
const char* kGeolocationProviderUrl =
    "https://www.googleapis.com/geolocation/v1/geolocate?key="
    GOOGLEAPIS_API_KEY;

}  // namespace

AtomAccessTokenStore::AtomAccessTokenStore() {
}

AtomAccessTokenStore::~AtomAccessTokenStore() {
}

void AtomAccessTokenStore::LoadAccessTokens(
    const LoadAccessTokensCallbackType& callback) {
  AccessTokenSet access_token_set;
  access_token_set[GURL(kGeolocationProviderUrl)];
  callback.Run(access_token_set,
               AtomBrowserContext::Get()->url_request_context_getter());
}

void AtomAccessTokenStore::SaveAccessToken(const GURL& server_url,
                                           const base::string16& access_token) {
}

}  // namespace atom

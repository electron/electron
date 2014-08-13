// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_access_token_store.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_url_request_context_getter.h"

namespace atom {

AtomAccessTokenStore::AtomAccessTokenStore() {
}

AtomAccessTokenStore::~AtomAccessTokenStore() {
}

void AtomAccessTokenStore::LoadAccessTokens(
    const LoadAccessTokensCallbackType& callback) {
  AccessTokenSet access_token_set;
  callback.Run(access_token_set,
               AtomBrowserContext::Get()->url_request_context_getter());
}

void AtomAccessTokenStore::SaveAccessToken(const GURL& server_url,
                                           const base::string16& access_token) {
}

}  // namespace atom

// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_ELECTRON_ACCESS_TOKEN_STORE_H_
#define ELECTRON_BROWSER_ELECTRON_ACCESS_TOKEN_STORE_H_

#include "content/public/browser/access_token_store.h"

namespace electron {

class AtomBrowserContext;

class AtomAccessTokenStore : public content::AccessTokenStore {
 public:
  AtomAccessTokenStore();
  virtual ~AtomAccessTokenStore();

  // content::AccessTokenStore:
  void LoadAccessTokens(
      const LoadAccessTokensCallbackType& callback) override;
  void SaveAccessToken(const GURL& server_url,
                       const base::string16& access_token) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomAccessTokenStore);
};

}  // namespace electron

#endif  // ELECTRON_BROWSER_ELECTRON_ACCESS_TOKEN_STORE_H_

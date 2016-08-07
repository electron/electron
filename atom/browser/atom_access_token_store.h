// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/access_token_store.h"

namespace atom {

class AtomAccessTokenStore : public content::AccessTokenStore {
 public:
  AtomAccessTokenStore();
  ~AtomAccessTokenStore();

  // content::AccessTokenStore:
  void LoadAccessTokens(
      const LoadAccessTokensCallback& callback) override;
  void SaveAccessToken(const GURL& server_url,
                       const base::string16& access_token) override;

 private:
  void GetRequestContextOnUIThread();
  void RespondOnOriginatingThread(const LoadAccessTokensCallback& callback);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(AtomAccessTokenStore);
};

}  // namespace atom

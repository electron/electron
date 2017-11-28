// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_COOKIE_DETAILS_H_
#define ATOM_BROWSER_NET_COOKIE_DETAILS_H_

#include "base/macros.h"
#include "net/cookies/cookie_store.h"

namespace atom {

struct CookieDetails {
 public:
  CookieDetails(const net::CanonicalCookie* cookie_copy,
                bool is_removed,
                net::CookieStore::ChangeCause cause)
      : cookie(cookie_copy), removed(is_removed), cause(cause) {}

  const net::CanonicalCookie* cookie;
  bool removed;
  net::CookieStore::ChangeCause cause;
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_COOKIE_DETAILS_H_

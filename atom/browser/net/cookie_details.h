// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_COOKIE_DETAILS_H_
#define ATOM_BROWSER_NET_COOKIE_DETAILS_H_

#include "base/macros.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace net {
class CanonicalCookie;
}

namespace atom {

struct CookieDetails {
 public:
  CookieDetails(const net::CanonicalCookie* cookie_copy,
                bool is_removed,
                network::mojom::CookieChangeCause cause)
      : cookie(cookie_copy), removed(is_removed), cause(cause) {}

  const net::CanonicalCookie* cookie;
  bool removed;
  network::mojom::CookieChangeCause cause;
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_COOKIE_DETAILS_H_

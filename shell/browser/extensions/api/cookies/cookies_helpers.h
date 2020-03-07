// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines common functionality used by the implementation of the Chrome
// Extensions Cookies API implemented in
// chrome/browser/extensions/api/cookies/cookies_api.cc. This separate interface
// exposes pieces of the API implementation mainly for unit testing purposes.

#ifndef CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_HELPERS_H_
#define CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_HELPERS_H_

#include <memory>
#include <string>
#include <vector>

#include "shell/common/extensions/api/cookies.h"

class Browser;
class Profile;

namespace base {
class ListValue;
}

namespace net {
class CanonicalCookie;
}

namespace extensions {

class Extension;

namespace cookies_helpers {

// Returns either the original profile or the incognito profile, based on the
// given store ID.  Returns NULL if the profile doesn't exist or is not allowed
// (e.g. if incognito mode is not enabled for the extension).
Profile* ChooseProfileFromStoreId(const std::string& store_id,
                                  Profile* profile,
                                  bool include_incognito);

// Returns the store ID for a particular user profile.
const char* GetStoreIdFromProfile(Profile* profile);

// Constructs a new Cookie object representing a cookie as defined by the
// cookies API.
api::cookies::Cookie CreateCookie(const net::CanonicalCookie& cookie,
                                  const std::string& store_id);

// Constructs a new CookieStore object as defined by the cookies API.
api::cookies::CookieStore CreateCookieStore(
    Profile* profile,
    std::unique_ptr<base::ListValue> tab_ids);

// Dispatch a request to the CookieManager for cookies associated with
// |url|.
void GetCookieListFromManager(
    network::mojom::CookieManager* manager,
    const GURL& url,
    network::mojom::CookieManager::GetCookieListCallback callback);

// Dispatch a request to the CookieManager for all cookies.
void GetAllCookiesFromManager(
    network::mojom::CookieManager* manager,
    network::mojom::CookieManager::GetAllCookiesCallback callback);

// Constructs a URL from a cookie's information for use in checking
// a cookie against the extension's host permissions. The Secure
// property of the cookie defines the URL scheme, and the cookie's
// domain becomes the URL host.
GURL GetURLFromCanonicalCookie(const net::CanonicalCookie& cookie);

// Looks through all cookies in the given cookie store, and appends to the
// match vector all the cookies that both match the given URL and cookie details
// and are allowed by extension host permissions.
void AppendMatchingCookiesFromCookieListToVector(
    const net::CookieList& all_cookies,
    const api::cookies::GetAll::Params::Details* details,
    const Extension* extension,
    std::vector<api::cookies::Cookie>* match_vector);

// Same as above except takes a CookieStatusList (and ignores the statuses).
void AppendMatchingCookiesFromCookieStatusListToVector(
    const net::CookieStatusList& all_cookies_with_statuses,
    const api::cookies::GetAll::Params::Details* details,
    const Extension* extension,
    std::vector<api::cookies::Cookie>* match_vector);

// Appends the IDs of all tabs belonging to the given browser to the
// given list.
void AppendToTabIdList(Browser* browser, base::ListValue* tab_ids);

// A class representing the cookie filter parameters passed into
// cookies.getAll().
// This class is essentially a convenience wrapper for the details dictionary
// passed into the cookies.getAll() API by the user. If the dictionary contains
// no filter parameters, the MatchFilter will always trivially
// match all cookies.
class MatchFilter {
 public:
  // Takes the details dictionary argument given by the user as input.
  // This class does not take ownership of the lifetime of the Details
  // object.
  explicit MatchFilter(const api::cookies::GetAll::Params::Details* details);

  // Returns true if the given cookie matches the properties in the match
  // filter.
  bool MatchesCookie(const net::CanonicalCookie& cookie);

 private:
  // Returns true if the given cookie domain string matches the filter's
  // domain. Any cookie domain which is equal to or is a subdomain of the
  // filter's domain will be matched; leading '.' characters indicating
  // host-only domains have no meaning in the match filter domain (for
  // instance, a match filter domain of 'foo.bar.com' will be treated the same
  // as '.foo.bar.com', and both will match cookies with domain values of
  // 'foo.bar.com', '.foo.bar.com', and 'baz.foo.bar.com'.
  bool MatchesDomain(const std::string& domain);

  const api::cookies::GetAll::Params::Details* details_;
};

}  // namespace cookies_helpers
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_HELPERS_H_
// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_navigation_throttle.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "content/public/browser/navigation_handle.h"

namespace atom {

AtomNavigationThrottle::AtomNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

AtomNavigationThrottle::~AtomNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
AtomNavigationThrottle::WillRedirectRequest() {
  auto* handle = navigation_handle();
  auto* contents = handle->GetWebContents();
  if (!contents)
    return PROCEED;

  auto api_contents =
      atom::api::WebContents::CreateFrom(v8::Isolate::GetCurrent(), contents);
  if (api_contents.IsEmpty())
    return PROCEED;

  if (api_contents->EmitNavigationEvent("will-redirect", handle)) {
    return CANCEL;
  }
  return PROCEED;
}

}  // namespace atom

// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_navigation_throttle.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "content/public/browser/navigation_handle.h"

namespace atom {

AtomNavigationThrottle::AtomNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

AtomNavigationThrottle::~AtomNavigationThrottle() {}

const char* AtomNavigationThrottle::GetNameForLogging() {
  return "AtomNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
AtomNavigationThrottle::WillRedirectRequest() {
  auto* handle = navigation_handle();
  auto* contents = handle->GetWebContents();
  if (!contents) {
    NOTREACHED();
    return PROCEED;
  }

  auto api_contents =
      atom::api::WebContents::From(v8::Isolate::GetCurrent(), contents);
  if (api_contents.IsEmpty()) {
    // No need to emit any event if the WebContents is not available in JS.
    return PROCEED;
  }

  if (api_contents->EmitNavigationEvent("will-redirect", handle)) {
    return CANCEL;
  }
  return PROCEED;
}

}  // namespace atom

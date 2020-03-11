// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_navigation_throttle.h"

#include "content/public/browser/navigation_handle.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace electron {

ElectronNavigationThrottle::ElectronNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

ElectronNavigationThrottle::~ElectronNavigationThrottle() = default;

const char* ElectronNavigationThrottle::GetNameForLogging() {
  return "ElectronNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
ElectronNavigationThrottle::WillRedirectRequest() {
  auto* handle = navigation_handle();
  auto* contents = handle->GetWebContents();
  if (!contents) {
    NOTREACHED();
    return PROCEED;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(isolate);
  auto api_contents = electron::api::WebContents::From(isolate, contents);
  if (api_contents.IsEmpty()) {
    // No need to emit any event if the WebContents is not available in JS.
    return PROCEED;
  }

  if (api_contents->EmitNavigationEvent("will-redirect", handle)) {
    return CANCEL;
  }
  return PROCEED;
}

}  // namespace electron

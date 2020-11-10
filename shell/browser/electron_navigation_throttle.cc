// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_navigation_throttle.h"

#include <memory>
#include <string>

#include "content/public/browser/navigation_handle.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"

namespace electron {

namespace {

bool HandleEventReturnValue(int* error_code,
                            std::string* error_html,
                            v8::Local<v8::Value> val) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict;
  gin::ConvertFromV8(isolate, val, &dict);
  if (!dict.Get("errorCode", error_code))
    return false;
  dict.Get("errorPage", error_html);
  return true;
}

}  // namespace

ElectronNavigationThrottle::ElectronNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

ElectronNavigationThrottle::~ElectronNavigationThrottle() = default;

const char* ElectronNavigationThrottle::GetNameForLogging() {
  return "ElectronNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
ElectronNavigationThrottle::DelegateEventToWebContents(
    const std::string& event_name,
    net::Error error_code) {
  auto* handle = navigation_handle();
  auto* contents = handle->GetWebContents();
  if (!contents) {
    NOTREACHED();
    return PROCEED;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  api::WebContents* api_contents = api::WebContents::From(contents);
  if (!api_contents) {
    // No need to emit any event if the WebContents is not available in JS.
    return PROCEED;
  }

  int error_code_int = error_code;
  std::string error_html;
  // JS ensures that this callback can not be called out-of-stack
  if (api_contents->EmitNavigationEvent(
          event_name, handle,
          base::BindOnce(&HandleEventReturnValue,
                         base::Unretained(&error_code_int),
                         base::Unretained(&error_html)))) {
    if (!error_html.empty()) {
      return content::NavigationThrottle::ThrottleCheckResult(
          CANCEL, error_code, error_html);
    }
    return content::NavigationThrottle::ThrottleCheckResult(CANCEL, error_code);
  }
  return PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
ElectronNavigationThrottle::WillFailRequest() {
  return DelegateEventToWebContents("-will-fail-load",
                                    navigation_handle()->GetNetErrorCode());
}

content::NavigationThrottle::ThrottleCheckResult
ElectronNavigationThrottle::WillStartRequest() {
  auto* handle = navigation_handle();
  if (handle->IsRendererInitiated() && handle->IsInMainFrame())
    return DelegateEventToWebContents("-will-navigate", net::ERR_ABORTED);
  return PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
ElectronNavigationThrottle::WillRedirectRequest() {
  auto* handle = navigation_handle();
  auto* contents = handle->GetWebContents();
  if (!contents) {
    NOTREACHED();
    return PROCEED;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  api::WebContents* api_contents = api::WebContents::From(contents);
  if (!api_contents) {
    // No need to emit any event if the WebContents is not available in JS.
    return PROCEED;
  }

  if (api_contents->EmitNavigationEvent("will-redirect", handle)) {
    return CANCEL;
  }
  return PROCEED;
}

}  // namespace electron

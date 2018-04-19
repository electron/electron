// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SECURITY_STATE_TAB_HELPER_H_
#define CHROME_BROWSER_SSL_SECURITY_STATE_TAB_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/WebKit/public/platform/WebSecurityStyle.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

// Tab helper provides the page's security status. Also logs console warnings
// for private data on insecure pages.
class SecurityStateTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SecurityStateTabHelper> {
 public:
  ~SecurityStateTabHelper() override;

  // See security_state::GetSecurityInfo.
  void GetSecurityInfo(security_state::SecurityInfo* result) const;

  // Called when the NavigationEntry's SSLStatus or other security
  // information changes.
  void VisibleSecurityStateChanged();

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

 private:
  explicit SecurityStateTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<SecurityStateTabHelper>;

  bool UsedPolicyInstalledCertificate() const;
#if 0
  security_state::MaliciousContentStatus GetMaliciousContentStatus() const;
#endif
  std::unique_ptr<security_state::VisibleSecurityState>
  GetVisibleSecurityState() const;

  // True if a console message has been logged about an omnibox warning that
  // will be shown in future versions of Chrome for insecure HTTP pages. This
  // message should only be logged once per main-frame navigation.
  bool logged_http_warning_on_current_navigation_;

  // The time that a console or omnibox warning was shown for insecure
  // HTTP pages that contain password or credit card fields. This is set
  // at most once per main-frame navigation (the first time that an HTTP
  // warning triggers on that navigation) and is used for UMA
  // histogramming.
  base::Time time_of_http_warning_on_current_navigation_;

  DISALLOW_COPY_AND_ASSIGN(SecurityStateTabHelper);
};

#endif  // CHROME_BROWSER_SSL_SECURITY_STATE_TAB_HELPER_H_

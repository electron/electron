// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/security_state_tab_helper.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "build/build_config.h"
#if 0
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/policy_cert_service.h"
#include "chrome/browser/chromeos/policy/policy_cert_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#endif
#include "components/prefs/pref_service.h"
#include "components/security_state/content/content_utils.h"
#if 0
#include "components/ssl_config/ssl_config_prefs.h"
#endif
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "net/base/net_errors.h"
#include "net/cert/x509_certificate.h"
#include "net/ssl/ssl_cipher_suite_names.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
#include "ui/base/l10n/l10n_util.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SecurityStateTabHelper);

#if 0
using safe_browsing::SafeBrowsingUIManager;
#endif

SecurityStateTabHelper::SecurityStateTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      logged_http_warning_on_current_navigation_(false) {}

SecurityStateTabHelper::~SecurityStateTabHelper() {}

void SecurityStateTabHelper::GetSecurityInfo(
    security_state::SecurityInfo* result) const {
  security_state::GetSecurityInfo(GetVisibleSecurityState(),
                                  UsedPolicyInstalledCertificate(),
                                  base::Bind(&content::IsOriginSecure), result);
}

void SecurityStateTabHelper::VisibleSecurityStateChanged() {
  if (logged_http_warning_on_current_navigation_)
    return;

  security_state::SecurityInfo security_info;
  GetSecurityInfo(&security_info);
  if (!security_info.displayed_password_field_on_http &&
      !security_info.displayed_credit_card_field_on_http) {
    return;
  }

  DCHECK(time_of_http_warning_on_current_navigation_.is_null());
  time_of_http_warning_on_current_navigation_ = base::Time::Now();

  std::string warning;
  bool warning_is_user_visible = false;
  switch (security_info.security_level) {
    case security_state::HTTP_SHOW_WARNING:
      warning =
          "This page includes a password or credit card input in a non-secure "
          "context. A warning has been added to the URL bar. For more "
          "information, see https://goo.gl/zmWq3m.";
      warning_is_user_visible = true;
      break;
    case security_state::NONE:
    case security_state::DANGEROUS:
      warning =
          "This page includes a password or credit card input in a non-secure "
          "context. A warning will be added to the URL bar in Chrome 56 (Jan "
          "2017). For more information, see https://goo.gl/zmWq3m.";
      break;
    default:
      return;
  }

  logged_http_warning_on_current_navigation_ = true;
  web_contents()->GetMainFrame()->AddMessageToConsole(
      content::CONSOLE_MESSAGE_LEVEL_WARNING, warning);

  if (security_info.displayed_credit_card_field_on_http) {
    UMA_HISTOGRAM_BOOLEAN(
        "Security.HTTPBad.UserWarnedAboutSensitiveInput.CreditCard",
        warning_is_user_visible);
  }
  if (security_info.displayed_password_field_on_http) {
    UMA_HISTOGRAM_BOOLEAN(
        "Security.HTTPBad.UserWarnedAboutSensitiveInput.Password",
        warning_is_user_visible);
  }
}

void SecurityStateTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (time_of_http_warning_on_current_navigation_.is_null() ||
      !navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  // Record how quickly a user leaves a site after encountering an
  // HTTP-bad warning. A navigation here only counts if it is a
  // main-frame, not-same-page navigation, since it aims to measure how
  // quickly a user leaves a site after seeing the HTTP warning.
  UMA_HISTOGRAM_LONG_TIMES(
      "Security.HTTPBad.NavigationStartedAfterUserWarnedAboutSensitiveInput",
      base::Time::Now() - time_of_http_warning_on_current_navigation_);
  // After recording the histogram, clear the time of the warning. A
  // timing histogram will not be recorded again on this page, because
  // the time is only set the first time the HTTP-bad warning is shown
  // per page.
  time_of_http_warning_on_current_navigation_ = base::Time();
}

void SecurityStateTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() &&
     !navigation_handle->IsSameDocument()) {
    // Only reset the console message flag for main-frame navigations,
    // and not for same-page navigations like reference fragments and pushState.
    logged_http_warning_on_current_navigation_ = false;
  }
}

void SecurityStateTabHelper::WebContentsDestroyed() {
  if (time_of_http_warning_on_current_navigation_.is_null()) {
    return;
  }
  // Record how quickly the tab is closed after a user encounters an
  // HTTP-bad warning. This histogram will only be recorded if the
  // WebContents is destroyed before another navigation begins.
  UMA_HISTOGRAM_LONG_TIMES(
      "Security.HTTPBad.WebContentsDestroyedAfterUserWarnedAboutSensitiveInput",
      base::Time::Now() - time_of_http_warning_on_current_navigation_);
}

bool SecurityStateTabHelper::UsedPolicyInstalledCertificate() const {
#if defined(OS_CHROMEOS)
  policy::PolicyCertService* service =
      policy::PolicyCertServiceFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  if (service && service->UsedPolicyCertificates())
    return true;
#endif
  return false;
}

#if 0
security_state::MaliciousContentStatus
SecurityStateTabHelper::GetMaliciousContentStatus() const {
  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  if (!entry)
    return security_state::MALICIOUS_CONTENT_STATUS_NONE;
  safe_browsing::SafeBrowsingService* sb_service =
      g_browser_process->safe_browsing_service();
  if (!sb_service)
    return security_state::MALICIOUS_CONTENT_STATUS_NONE;
  scoped_refptr<SafeBrowsingUIManager> sb_ui_manager = sb_service->ui_manager();
  safe_browsing::SBThreatType threat_type;
  if (sb_ui_manager->IsUrlWhitelistedOrPendingForWebContents(
          entry->GetURL(), false, entry, web_contents(), false, &threat_type)) {
    switch (threat_type) {
      case safe_browsing::SB_THREAT_TYPE_UNUSED:
      case safe_browsing::SB_THREAT_TYPE_SAFE:
        break;
      case safe_browsing::SB_THREAT_TYPE_URL_PHISHING:
      case safe_browsing::SB_THREAT_TYPE_CLIENT_SIDE_PHISHING_URL:
        return security_state::MALICIOUS_CONTENT_STATUS_SOCIAL_ENGINEERING;
        break;
      case safe_browsing::SB_THREAT_TYPE_URL_MALWARE:
      case safe_browsing::SB_THREAT_TYPE_CLIENT_SIDE_MALWARE_URL:
        return security_state::MALICIOUS_CONTENT_STATUS_MALWARE;
        break;
      case safe_browsing::SB_THREAT_TYPE_URL_UNWANTED:
        return security_state::MALICIOUS_CONTENT_STATUS_UNWANTED_SOFTWARE;
        break;
      case safe_browsing::SB_THREAT_TYPE_BINARY_MALWARE_URL:
      case safe_browsing::SB_THREAT_TYPE_EXTENSION:
      case safe_browsing::SB_THREAT_TYPE_BLACKLISTED_RESOURCE:
      case safe_browsing::SB_THREAT_TYPE_API_ABUSE:
        // These threat types are not currently associated with
        // interstitials, and thus resources with these threat types are
        // not ever whitelisted or pending whitelisting.
        NOTREACHED();
        break;
    }
  }
  return security_state::MALICIOUS_CONTENT_STATUS_NONE;
}
#endif

std::unique_ptr<security_state::VisibleSecurityState>
SecurityStateTabHelper::GetVisibleSecurityState() const {
  auto state = security_state::GetVisibleSecurityState(web_contents());

#if 0
  // Malware status might already be known even if connection security
  // information is still being initialized, thus no need to check for that.
  state->malicious_content_status = GetMaliciousContentStatus();
#endif

  return state;
}

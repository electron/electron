// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/browser/net/require_ct_delegate.h"

#include "content/public/browser/browser_thread.h"

namespace brightray {

RequireCTDelegate::RequireCTDelegate() {}

RequireCTDelegate::~RequireCTDelegate() {}

void RequireCTDelegate::AddCTExcludedHost(const std::string& host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  ct_excluded_hosts_.insert(host);
}

void RequireCTDelegate::ClearCTExcludedHostsList() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  ct_excluded_hosts_.clear();
}

RequireCTDelegate::CTRequirementLevel RequireCTDelegate::IsCTRequiredForHost(
    const std::string& host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!ct_excluded_hosts_.empty() &&
      (ct_excluded_hosts_.find(host) != ct_excluded_hosts_.end()))
    return CTRequirementLevel::NOT_REQUIRED;
  return CTRequirementLevel::DEFAULT;
}

}  // namespace brightray

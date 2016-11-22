// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_ct_delegate.h"

#include "content/public/browser/browser_thread.h"

namespace atom {

AtomCTDelegate::AtomCTDelegate() {}

AtomCTDelegate::~AtomCTDelegate() {}

void AtomCTDelegate::AddCTExcludedHost(const std::string& host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  ct_excluded_hosts_.insert(host);
}

void AtomCTDelegate::ClearCTExcludedHostsList() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  ct_excluded_hosts_.clear();
}

AtomCTDelegate::CTRequirementLevel AtomCTDelegate::IsCTRequiredForHost(
    const std::string& host) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!ct_excluded_hosts_.empty() &&
      (ct_excluded_hosts_.find(host) != ct_excluded_hosts_.end()))
    return CTRequirementLevel::NOT_REQUIRED;
  return CTRequirementLevel::DEFAULT;
}

}  // namespace atom

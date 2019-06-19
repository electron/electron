// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_SPECIAL_STORAGE_POLICY_H_
#define ATOM_BROWSER_SPECIAL_STORAGE_POLICY_H_

#include "storage/browser/quota/special_storage_policy.h"

namespace atom {

class SpecialStoragePolicy : public storage::SpecialStoragePolicy {
 public:
  SpecialStoragePolicy();

  // storage::SpecialStoragePolicy implementation.
  bool IsStorageProtected(const GURL& origin) override;
  bool IsStorageUnlimited(const GURL& origin) override;
  bool IsStorageDurable(const GURL& origin) override;
  bool HasIsolatedStorage(const GURL& origin) override;
  bool IsStorageSessionOnly(const GURL& origin) override;
  bool HasSessionOnlyOrigins() override;
  network::SessionCleanupCookieStore::DeleteCookiePredicate
  CreateDeleteCookieOnExitPredicate() override;

 protected:
  ~SpecialStoragePolicy() override;
};

}  // namespace atom

#endif  // ATOM_BROWSER_SPECIAL_STORAGE_POLICY_H_

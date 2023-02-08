// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/special_storage_policy.h"

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "services/network/public/cpp/session_cookie_delete_predicate.h"

namespace electron {

SpecialStoragePolicy::SpecialStoragePolicy() = default;

SpecialStoragePolicy::~SpecialStoragePolicy() = default;

bool SpecialStoragePolicy::IsStorageProtected(const GURL& origin) {
  return true;
}

bool SpecialStoragePolicy::IsStorageUnlimited(const GURL& origin) {
  return true;
}

bool SpecialStoragePolicy::IsStorageDurable(const GURL& origin) {
  return true;
}

bool SpecialStoragePolicy::HasIsolatedStorage(const GURL& origin) {
  return false;
}

bool SpecialStoragePolicy::IsStorageSessionOnly(const GURL& origin) {
  return false;
}

bool SpecialStoragePolicy::HasSessionOnlyOrigins() {
  return false;
}

}  // namespace electron

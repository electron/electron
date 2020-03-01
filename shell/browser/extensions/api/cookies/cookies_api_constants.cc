// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/cookies/cookies_api_constants.h"

namespace extensions {
namespace cookies_api_constants {

// Keys
const char kCauseKey[] = "cause";
const char kCookieKey[] = "cookie";
const char kDomainKey[] = "domain";
const char kIdKey[] = "id";
const char kRemovedKey[] = "removed";
const char kTabIdsKey[] = "tabIds";

// Cause Constants
const char kEvictedChangeCause[] = "evicted";
const char kExpiredChangeCause[] = "expired";
const char kExpiredOverwriteChangeCause[] = "expired_overwrite";
const char kExplicitChangeCause[] = "explicit";
const char kOverwriteChangeCause[] = "overwrite";

// Errors
const char kCookieSetFailedError[] =
    "Failed to parse or set cookie named \"*\".";
const char kInvalidStoreIdError[] = "Invalid cookie store id: \"*\".";
const char kInvalidUrlError[] = "Invalid url: \"*\".";
const char kNoCookieStoreFoundError[] =
    "No accessible cookie store found for the current execution context.";
const char kNoHostPermissionsError[] =
    "No host permissions for cookies at url: \"*\".";

}  // namespace cookies_api_constants
}  // namespace extensions
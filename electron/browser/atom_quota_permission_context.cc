// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_quota_permission_context.h"

#include "storage/common/quota/quota_types.h"

namespace atom {

AtomQuotaPermissionContext::AtomQuotaPermissionContext() {
}

AtomQuotaPermissionContext::~AtomQuotaPermissionContext() {
}

void AtomQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    const PermissionCallback& callback) {
  callback.Run(response::QUOTA_PERMISSION_RESPONSE_ALLOW);
}

}  // namespace atom

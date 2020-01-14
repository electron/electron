// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_quota_permission_context.h"

#include <utility>

namespace electron {

AtomQuotaPermissionContext::AtomQuotaPermissionContext() = default;

AtomQuotaPermissionContext::~AtomQuotaPermissionContext() = default;

void AtomQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    PermissionCallback callback) {
  std::move(callback).Run(response::QUOTA_PERMISSION_RESPONSE_ALLOW);
}

}  // namespace electron

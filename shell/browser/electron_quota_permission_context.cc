// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_quota_permission_context.h"

#include <utility>

#include "content/public/common/storage_quota_params.h"

namespace electron {

ElectronQuotaPermissionContext::ElectronQuotaPermissionContext() = default;

ElectronQuotaPermissionContext::~ElectronQuotaPermissionContext() = default;

void ElectronQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    PermissionCallback callback) {
  std::move(callback).Run(response::QUOTA_PERMISSION_RESPONSE_ALLOW);
}

}  // namespace electron

// Copyright (c) 2023 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_storage_access.h"

#include <string>

// using Browser::TopLevelStorage;

namespace electron::api {

void TopLevelStorageAccessPermissionContext::DecidePermission(
    permissions::PermissionRequestData request_data,
    permissions::BrowserPermissionCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      request_data.id.global_render_frame_host_id());
  CHECK(rfh);
  if (!request_data.user_gesture ||
      !base::FeatureList::IsEnabled(
          blink::features::kStorageAccessAPIForOriginExtension) ||
      !request_data.requesting_origin.is_valid() ||
      !request_data.embedding_origin.is_valid()) {
    if (!request_data.user_gesture) {
      rfh->AddMessageToConsole(
          blink::mojom::ConsoleMessageLevel::kError,
          "requestStorageAccessFor: Must be handling a user gesture to use.");
    }
    RecordOutcomeSample(
        TopLevelStorageAccessRequestOutcome::kDeniedByPrerequisites);
    std::move(callback).Run(CONTENT_SETTING_BLOCK);
    return;
  }

  // if (!base::FeatureList::IsEnabled(features::kFirstPartySets)) {
  //   // First-Party Sets is disabled, so reject the request.
  //   RecordOutcomeSample(
  //       TopLevelStorageAccessRequestOutcome::kDeniedByPrerequisites);
  //   std::move(callback).Run(CONTENT_SETTING_BLOCK);
  //   return;
  // }

  net::SchemefulSite embedding_site(request_data.embedding_origin);
  net::SchemefulSite requesting_site(request_data.requesting_origin);

  first_party_sets::FirstPartySetsPolicyServiceFactory::GetForBrowserContext(
      browser_context())
      ->ComputeFirstPartySetMetadata(
          requesting_site, &embedding_site,
          base::BindOnce(&TopLevelStorageAccessPermissionContext::
                             CheckForAutoGrantOrAutoDenial,
                         weak_factory_.GetWeakPtr(), std::move(request_data),
                         std::move(callback)));
}

}  // namespace electron::api
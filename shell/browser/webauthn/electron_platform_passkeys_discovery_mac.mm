// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/webauthn/electron_platform_passkeys_discovery.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/render_frame_host.h"
#include "shell/browser/webauthn/electron_platform_passkeys_authenticator.h"

namespace electron {

ElectronPlatformPasskeysDiscovery::ElectronPlatformPasskeysDiscovery(
    content::RenderFrameHost* render_frame_host)
    : FidoDiscoveryBase(device::FidoTransportProtocol::kInternal),
      render_frame_host_token_(render_frame_host->GetGlobalFrameToken()) {}

ElectronPlatformPasskeysDiscovery::~ElectronPlatformPasskeysDiscovery() =
    default;

void ElectronPlatformPasskeysDiscovery::Start() {
  if (!observer()) {
    return;
  }

  // DiscoveryStarted must be invoked asynchronously from Start().
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ElectronPlatformPasskeysDiscovery::OnStartComplete,
                     weak_factory_.GetWeakPtr()));
}

void ElectronPlatformPasskeysDiscovery::OnStartComplete() {
  if (!ElectronPlatformPasskeysAuthenticator::IsAvailable()) {
    observer()->DiscoveryStarted(this, /*success=*/false);
    return;
  }

  authenticator_ = std::make_unique<ElectronPlatformPasskeysAuthenticator>(
      render_frame_host_token_);
  observer()->DiscoveryStarted(this, /*success=*/true, {authenticator_.get()});
}

}  // namespace electron

// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_PLATFORM_PASSKEYS_DISCOVERY_H_
#define ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_PLATFORM_PASSKEYS_DISCOVERY_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/global_routing_id.h"
#include "device/fido/fido_discovery_base.h"

namespace content {
class RenderFrameHost;
}

namespace electron {

class ElectronPlatformPasskeysAuthenticator;

class ElectronPlatformPasskeysDiscovery : public device::FidoDiscoveryBase {
 public:
  explicit ElectronPlatformPasskeysDiscovery(
      content::RenderFrameHost* render_frame_host);
  ~ElectronPlatformPasskeysDiscovery() override;

  ElectronPlatformPasskeysDiscovery(const ElectronPlatformPasskeysDiscovery&) =
      delete;
  ElectronPlatformPasskeysDiscovery& operator=(
      const ElectronPlatformPasskeysDiscovery&) = delete;

  // device::FidoDiscoveryBase:
  void Start() override;

 private:
  void OnStartComplete();

  const content::GlobalRenderFrameHostToken render_frame_host_token_;
  std::unique_ptr<ElectronPlatformPasskeysAuthenticator> authenticator_;
  base::WeakPtrFactory<ElectronPlatformPasskeysDiscovery> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEBAUTHN_ELECTRON_PLATFORM_PASSKEYS_DISCOVERY_H_

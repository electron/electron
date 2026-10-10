// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_DEVICE_BOUND_SESSIONS_H_
#define ELECTRON_SHELL_BROWSER_NET_DEVICE_BOUND_SESSIONS_H_

#include <memory>
#include <optional>

#include "base/files/file_path.h"
#include "components/unexportable_keys/mojom/unexportable_key_service.mojom-forward.h"
#include "crypto/unexportable_key.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace unexportable_keys {
class UnexportableKeyService;
class UnexportableKeyServiceProxyImpl;
}  // namespace unexportable_keys

namespace electron {

// Returns true if a network context should support Device Bound Session
// Credentials (DBSC).
//
// DBSC is on when the `deviceBoundSessions` fuse is enabled. With the fuse off
// it is on only if mock software keys were explicitly requested through the
// EnableBoundSessionCredentialsSoftwareKeysForManualTesting feature, which is
// the supported way to exercise DBSC in development where hardware-backed keys
// are unavailable. InitializeFeatureList() removes that feature whenever the
// fuse is on, so the two paths never overlap in a fused app.
//
// `in_memory` is whether the context has no on-disk storage. On macOS,
// hardware-backed keys live in the Keychain and outlast the process, so an
// in-memory context would orphan them; DBSC is not enabled there.
bool ShouldEnableDeviceBoundSessions(bool in_memory);

// Builds the unexportable key provider configuration for the storage partition
// at `partition_path`, or std::nullopt if this app cannot use DBSC key storage
// on this platform (the reason is logged).
//
// On macOS the config carries a keychain access group derived from the app's
// own code signature (<TeamID>.<BundleID>.unexportable-keys), which the app
// must list in its `keychain-access-groups` entitlement, plus an application
// tag scoped to the partition so that its keys can be found and deleted
// together. `software_keys` requests mock keys that never touch the Keychain,
// so the code signature is not required. On other platforms the config is
// empty.
std::optional<crypto::UnexportableKeyProvider::Config>
GetDeviceBoundSessionsKeyProviderConfig(const base::FilePath& partition_path,
                                        bool software_keys);

// Hosts the unexportable key service that a partition's network service uses
// for DBSC, in the browser process.
//
// Running key operations here rather than in the network service matters on
// macOS, where the network service is sandboxed and its built-in key provider
// hardcodes Chromium's keychain access group, which no Electron app can hold.
// It matches what Chrome does behind kUseUnexportableKeyServiceInBrowserProcess
// (chrome/browser/net/profile_network_context_service.cc).
class DeviceBoundSessionsKeyService {
 public:
  explicit DeviceBoundSessionsKeyService(base::FilePath partition_path);
  ~DeviceBoundSessionsKeyService();

  DeviceBoundSessionsKeyService(const DeviceBoundSessionsKeyService&) = delete;
  DeviceBoundSessionsKeyService& operator=(
      const DeviceBoundSessionsKeyService&) = delete;

  // Returns a remote for
  // NetworkContextParams::bound_sessions_unexportable_key_service that is
  // connected to this service, or an invalid remote if no key provider is
  // available. Each call rebinds the service, so the previous remote is
  // disconnected; that is what a restarted network service needs.
  mojo::PendingRemote<unexportable_keys::mojom::UnexportableKeyService>
  BindNewRemote();

 private:
  const base::FilePath partition_path_;

  // Declared before `proxy_` because the proxy holds a pointer to it.
  std::unique_ptr<unexportable_keys::UnexportableKeyService> service_;
  std::unique_ptr<unexportable_keys::UnexportableKeyServiceProxyImpl> proxy_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_DEVICE_BOUND_SESSIONS_H_

// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/device_bound_sessions.h"

#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "components/unexportable_keys/background_task_origin.h"
#include "components/unexportable_keys/features.h"
#include "components/unexportable_keys/mojom/unexportable_key_service_proxy_impl.h"
#include "components/unexportable_keys/unexportable_key_service_impl.h"
#include "components/unexportable_keys/unexportable_key_task_manager.h"
#include "electron/fuses.h"

namespace electron {

namespace {

bool AreSoftwareKeysEnabled() {
  return base::FeatureList::IsEnabled(
      unexportable_keys::
          kEnableBoundSessionCredentialsSoftwareKeysForManualTesting);
}

// Returns the task manager shared by every partition's key service, or nullptr
// if this device has no unexportable key provider. Support is decided once, on
// the first call: a provider's availability does not depend on which partition
// asks, and creating the task manager is what makes the (potentially slow)
// provider probe happen.
unexportable_keys::UnexportableKeyTaskManager* GetSharedTaskManager(
    const crypto::UnexportableKeyProvider::Config& config) {
  static base::NoDestructor<
      std::unique_ptr<unexportable_keys::UnexportableKeyTaskManager>>
      instance(unexportable_keys::UnexportableKeyServiceImpl::
                       IsUnexportableKeyProviderSupported(config)
                   ? std::make_unique<
                         unexportable_keys::UnexportableKeyTaskManager>()
                   : nullptr);
  return instance->get();
}

}  // namespace

bool ShouldEnableDeviceBoundSessions(bool in_memory) {
  const bool software_keys = AreSoftwareKeysEnabled();

  if (electron::fuses::IsDeviceBoundSessionsEnabled()) {
    // InitializeFeatureList() removes the software keys feature when the fuse
    // is on, so this cannot happen. If it ever does, fail closed instead of
    // letting a fused app run DBSC on keys that are not hardware-backed.
    if (software_keys) {
      LOG(ERROR) << "Device Bound Sessions: refusing to enable software keys "
                    "because the deviceBoundSessions fuse is on.";
      return false;
    }
#if BUILDFLAG(IS_MAC)
    if (in_memory) {
      LOG(WARNING)
          << "Device Bound Sessions are not supported for in-memory sessions "
             "on macOS: their hardware-backed keys are stored in the Keychain "
             "and would outlive the session. Use a persistent partition.";
      return false;
    }
#endif
    return true;
  }

  return software_keys;
}

#if !BUILDFLAG(IS_MAC)
std::optional<crypto::UnexportableKeyProvider::Config>
GetDeviceBoundSessionsKeyProviderConfig(const base::FilePath& partition_path,
                                        bool software_keys) {
  // The provider config only has members on Apple platforms.
  return crypto::UnexportableKeyProvider::Config{};
}
#endif

DeviceBoundSessionsKeyService::DeviceBoundSessionsKeyService(
    base::FilePath partition_path)
    : partition_path_(std::move(partition_path)) {}

DeviceBoundSessionsKeyService::~DeviceBoundSessionsKeyService() = default;

mojo::PendingRemote<unexportable_keys::mojom::UnexportableKeyService>
DeviceBoundSessionsKeyService::BindNewRemote() {
  // Disconnect the previous network service, if any, before rebinding.
  proxy_.reset();

  if (!service_) {
    std::optional<crypto::UnexportableKeyProvider::Config> config =
        GetDeviceBoundSessionsKeyProviderConfig(partition_path_,
                                                AreSoftwareKeysEnabled());
    if (!config) {
      return {};
    }
    unexportable_keys::UnexportableKeyTaskManager* task_manager =
        GetSharedTaskManager(*config);
    if (!task_manager) {
      LOG(WARNING)
          << "Device Bound Sessions: no hardware-backed key provider is "
             "available on this device (Windows needs a TPM 2.0, macOS needs "
             "a Secure Enclave), so sessions will not be registered.";
      return {};
    }
    service_ = std::make_unique<unexportable_keys::UnexportableKeyServiceImpl>(
        *task_manager,
        unexportable_keys::BackgroundTaskOrigin::kDeviceBoundSessionCredentials,
        std::move(*config));
  }

  mojo::PendingRemote<unexportable_keys::mojom::UnexportableKeyService> remote;
  proxy_ = std::make_unique<unexportable_keys::UnexportableKeyServiceProxyImpl>(
      service_.get(), remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

}  // namespace electron

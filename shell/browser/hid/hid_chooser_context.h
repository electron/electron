// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/queue.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/unguessable_token.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "shell/browser/electron_browser_context.h"
#include "url/origin.h"

namespace base {
class Value;
}

namespace electron {

extern const char kHidDeviceNameKey[];
extern const char kHidGuidKey[];
extern const char kHidVendorIdKey[];
extern const char kHidProductIdKey[];
extern const char kHidSerialNumberKey[];

// Manages the internal state and connection to the device service for the
// Human Interface Device (HID) chooser UI.
class HidChooserContext : public KeyedService,
                          public device::mojom::HidManagerClient {
 public:
  // This observer can be used to be notified when HID devices are connected or
  // disconnected.
  class DeviceObserver : public base::CheckedObserver {
   public:
    virtual void OnDeviceAdded(const device::mojom::HidDeviceInfo&) = 0;
    virtual void OnDeviceRemoved(const device::mojom::HidDeviceInfo&) = 0;
    virtual void OnDeviceChanged(const device::mojom::HidDeviceInfo&) = 0;
    virtual void OnHidManagerConnectionError() = 0;

    // Called when the HidChooserContext is shutting down. Observers must remove
    // themselves before returning.
    virtual void OnHidChooserContextShutdown() = 0;
  };

  explicit HidChooserContext(ElectronBrowserContext* context);
  HidChooserContext(const HidChooserContext&) = delete;
  HidChooserContext& operator=(const HidChooserContext&) = delete;
  ~HidChooserContext() override;

  // Returns a human-readable string identifier for |device|.
  static std::u16string DisplayNameFromDeviceInfo(
      const device::mojom::HidDeviceInfo& device);

  // Returns true if a persistent permission can be granted for |device|.
  static bool CanStorePersistentEntry(
      const device::mojom::HidDeviceInfo& device);

  static base::Value DeviceInfoToValue(
      const device::mojom::HidDeviceInfo& device);

  // HID-specific interface for granting and checking permissions.
  void GrantDevicePermission(const url::Origin& origin,
                             const device::mojom::HidDeviceInfo& device,
                             content::RenderFrameHost* render_frame_host);
  bool HasDevicePermission(const url::Origin& origin,
                           const device::mojom::HidDeviceInfo& device,
                           content::RenderFrameHost* render_frame_host);

  // For ScopedObserver.
  void AddDeviceObserver(DeviceObserver* observer);
  void RemoveDeviceObserver(DeviceObserver* observer);

  // Forward HidManager::GetDevices.
  void GetDevices(device::mojom::HidManager::GetDevicesCallback callback);

  // Only call this if you're sure |devices_| has been initialized before-hand.
  // The returned raw pointer is owned by |devices_| and will be destroyed when
  // the device is removed.
  const device::mojom::HidDeviceInfo* GetDeviceInfo(const std::string& guid);

  device::mojom::HidManager* GetHidManager();

  base::WeakPtr<HidChooserContext> AsWeakPtr();

 private:
  // device::mojom::HidManagerClient implementation:
  void DeviceAdded(device::mojom::HidDeviceInfoPtr device_info) override;
  void DeviceRemoved(device::mojom::HidDeviceInfoPtr device_info) override;
  void DeviceChanged(device::mojom::HidDeviceInfoPtr device_info) override;

  void EnsureHidManagerConnection();
  void SetUpHidManagerConnection(
      mojo::PendingRemote<device::mojom::HidManager> manager);
  void InitDeviceList(std::vector<device::mojom::HidDeviceInfoPtr> devices);
  void OnHidManagerInitializedForTesting(
      device::mojom::HidManager::GetDevicesCallback callback,
      std::vector<device::mojom::HidDeviceInfoPtr> devices);
  void OnHidManagerConnectionError();

  ElectronBrowserContext* browser_context_;

  bool is_initialized_ = false;
  base::queue<device::mojom::HidManager::GetDevicesCallback>
      pending_get_devices_requests_;

  // Tracks the set of devices to which an origin has access to.
  std::map<url::Origin, std::set<std::string>> ephemeral_devices_;

  // Map from device GUID to device info.
  std::map<std::string, device::mojom::HidDeviceInfoPtr> devices_;

  mojo::Remote<device::mojom::HidManager> hid_manager_;
  mojo::AssociatedReceiver<device::mojom::HidManagerClient> client_receiver_{
      this};
  base::ObserverList<DeviceObserver> device_observer_list_;

  base::WeakPtrFactory<HidChooserContext> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_H_
